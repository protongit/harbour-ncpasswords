#include "vaultcontroller.h"

#include "cryptoops.h"
#include "loginflow.h"
#include "ncclient.h"
#include "secretstore.h"
#include "sodiumutils.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QEventLoop>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QTimer>
#include <QUrl>

#include <algorithm>

namespace {

QString friendlyApiCode(const ApiResult &result)
{
    if (result.error == QLatin1String("unauthorized")) {
        return QStringLiteral("auth");
    }
    return result.error;
}

QString randomChars(const QString &pool, int count)
{
    const QByteArray random = Sodium::randomBytes(count * 2 + 8);
    if (random.isEmpty() || pool.isEmpty()) {
        return QString();
    }
    QString out;
    int index = 0;
    while (out.size() < count) {
        const unsigned char byte = static_cast<unsigned char>(random.at(index % random.size()));
        ++index;
        out.append(pool.at(byte % pool.size()));
    }
    return out;
}

bool validPin(const QString &pin)
{
    if (pin.size() < 4 || pin.size() > 6) {
        return false;
    }
    for (const QChar &c : pin) {
        if (!c.isDigit()) {
            return false;
        }
    }
    return true;
}

bool containsAny(const QString &text, const QString &characters)
{
    for (const QChar &c : characters) {
        if (text.contains(c)) {
            return true;
        }
    }
    return false;
}

} // namespace

static void logFingerprint(const QString &text)
{
    QFile file(SecretStore::directoryPath() + QStringLiteral("/fingerprint.log"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        file.write(QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8());
        file.write(" ");
        file.write(text.toUtf8());
        file.write("\n");
    }
}

static QString settingsPath()
{
    const QString directory = SecretStore::directoryPath();
    QDir().mkpath(directory);
    return directory + QStringLiteral("/settings.ini");
}

VaultController::VaultController(QObject *parent)
    : QObject(parent)
    , m_state(QStringLiteral("setup"))
    , m_passwords(new PasswordListModel(this))
    , m_trash(new PasswordListModel(this))
    , m_folderPasswords(new PasswordListModel(this))
    , m_lockTimer(new QTimer(this))
    , m_keepAliveTimer(new QTimer(this))
    , m_lockSeconds(300)
    , m_clipboardSeconds(30)
    , m_totpTimer(new QTimer(this))
    , m_loginPollTimer(new QTimer(this))
{
    Sodium::init();

    m_trash->setTrashMode(true);
    m_folderPasswords->setTrashMode(false);

    QSettings settings(settingsPath(), QSettings::IniFormat);
    if (settings.contains(QStringLiteral("lockSeconds"))) {
        m_lockSeconds = settings.value(QStringLiteral("lockSeconds"), 300).toInt();
    } else {
        m_lockSeconds = settings.value(QStringLiteral("lockMinutes"), 5).toInt() * 60;
    }
    if (m_lockSeconds < 0) {
        m_lockSeconds = 0;
    }
    m_clipboardSeconds = settings.value(QStringLiteral("clipboardSeconds"), 30).toInt();
    m_passwordStrength = settings.value(QStringLiteral("passwordStrength"),
                                       QStringLiteral("high")).toString();
    if (m_passwordStrength != QLatin1String("low")
            && m_passwordStrength != QLatin1String("medium")
            && m_passwordStrength != QLatin1String("high")
            && m_passwordStrength != QLatin1String("veryhigh")) {
        m_passwordStrength = QStringLiteral("high");
    }
    QCoreApplication::instance()->installEventFilter(this);
    // Fingerprint authentication is not available to sandboxed third-party
    // apps on this Sailfish OS version (the device-lock socket is privileged).
    m_fingerprintLock = false;
    SecretStore::setProtectedMode(false);

    m_pinEnabled = settings.value(QStringLiteral("pinEnabled"), false).toBool();
    m_pinSalt = Sodium::fromHex(settings.value(QStringLiteral("pinSalt")).toString());
    m_pinHash = Sodium::fromHex(settings.value(QStringLiteral("pinHash")).toString());
    if (m_pinSalt.size() != 16 || m_pinHash.size() != 32) {
        m_pinEnabled = false;
    }
    m_viewMode = settings.value(QStringLiteral("viewMode"), QStringLiteral("list")).toString();
    if (m_viewMode != QLatin1String("folders")) {
        m_viewMode = QStringLiteral("list");
    }
    m_language = settings.value(QStringLiteral("language"), QStringLiteral("system")).toString();
    if (m_language.isEmpty()) {
        m_language = QStringLiteral("system");
    }

    m_lockTimer->setSingleShot(true);
    connect(m_lockTimer, &QTimer::timeout, this, &VaultController::lock);

    m_keepAliveTimer->setInterval(5 * 60 * 1000);
    connect(m_keepAliveTimer, &QTimer::timeout, this, &VaultController::keepAlive);

    m_loginPollTimer->setInterval(2000);
    connect(m_loginPollTimer, &QTimer::timeout, this, &VaultController::pollLoginFlow);

    m_totpTimer->setInterval(1000);
    connect(m_totpTimer, &QTimer::timeout, this, [this]() {
        if (!m_totpServices.isEmpty()) {
            emit totpChanged();
        }
    });
    m_totpTimer->start();
}

VaultController::~VaultController()
{
}

QVariantList VaultController::folders() const
{
    QVariantList list;
    for (const Folder &folder : m_folders) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), folder.id);
        item.insert(QStringLiteral("revision"), folder.revision);
        item.insert(QStringLiteral("label"), folder.label);
        item.insert(QStringLiteral("parentId"), folder.parentId);
        item.insert(QStringLiteral("favorite"), folder.favorite);
        list.append(item);
    }
    return list;
}

QVariantList VaultController::tags() const
{
    QVariantList list;
    for (const Tag &tag : m_tags) {
        QVariantMap item;
        item.insert(QStringLiteral("id"), tag.id);
        item.insert(QStringLiteral("revision"), tag.revision);
        item.insert(QStringLiteral("label"), tag.label);
        item.insert(QStringLiteral("color"), tag.color);
        list.append(item);
    }
    return list;
}

void VaultController::setLockSeconds(int seconds)
{
    if (seconds < 0) {
        seconds = 0;
    }
    if (m_lockSeconds == seconds) {
        return;
    }
    m_lockSeconds = seconds;
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("lockSeconds"), seconds);
    settings.remove(QStringLiteral("lockMinutes"));
    emit lockSecondsChanged();
    noteActivity();
}

bool VaultController::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)
    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::TouchBegin:
    case QEvent::Wheel:
        noteActivity();
        break;
    default:
        break;
    }
    return false;
}

void VaultController::setClipboardSeconds(int seconds)
{
    if (seconds < 0) {
        seconds = 0;
    }
    if (m_clipboardSeconds == seconds) {
        return;
    }
    m_clipboardSeconds = seconds;
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("clipboardSeconds"), seconds);
    emit clipboardSecondsChanged();
}

void VaultController::restoreAccount()
{
    if (m_restored) {
        return;
    }
    m_restored = true;

    SecretStore::setProtectedMode(false);

    QString server;
    QString user;
    QString password;
    if (!SecretStore::load(&server, &user, &password)) {
        if (SecretStore::hasAccount() && !SecretStore::lastError().isEmpty()) {
            setError(QStringLiteral("fingerprint_failed"), SecretStore::lastError());
        }
        setState(QStringLiteral("setup"));
        return;
    }

    // Migrate the old custom-lock secret to the normal collection. The device
    // fingerprint prompt is provided by the QML Authenticator, not by Secrets.
    if (SecretStore::protectedMode()) {
        if (!password.isEmpty() && SecretStore::storeAppPassword(password)) {
            SecretStore::deleteProtected();
        }
        SecretStore::setProtectedMode(false);
    }

    m_server = server;
    m_user = user;
    m_appPassword = password;
    m_hasAccount = true;
    m_secretsActive = SecretStore::usingSecrets();
    m_fingerprintRequired = false;
    m_pinRequired = m_pinEnabled;
    emit secretsActiveChanged();
    emit fingerprintRequiredChanged();
    emit pinRequiredChanged();

    emit accountChanged();
    if (m_fingerprintRequired || m_pinRequired) {
        setState(QStringLiteral("locked"));
        return;
    }
    startSessionRequest();
}

void VaultController::startLoginFlow(const QString &server)
{
    clearError();
    setBusy(true);

    const QString serverUrl = server.trimmed();

    runTask<LoginFlowStart>([serverUrl]() {
        return LoginFlow::start(serverUrl);
    }, [this, serverUrl](const LoginFlowStart &startResult) {
        if (!startResult.ok) {
            setBusy(false);
            setError(startResult.error, startResult.detail);
            return;
        }

        emit loginFlowUrl(startResult.url);

        m_loginServer = serverUrl;
        m_loginEndpoint = startResult.endpoint;
        m_loginToken = startResult.token;
        m_loginFlowActive = true;
        emit loginFlowActiveChanged();
        m_loginPollTimer->start();
    });
}

void VaultController::pollLoginFlow()
{
    if (!m_loginFlowActive) {
        return;
    }

    const LoginFlowResult result = LoginFlow::pollOnce(m_loginEndpoint, m_loginToken);
    if (result.ok) {
        m_loginPollTimer->stop();
        m_loginFlowActive = false;
        emit loginFlowActiveChanged();
        setBusy(false);
        connectToServer(m_loginServer, result.loginName, result.appPassword);
    } else if (!result.pending) {
        m_loginPollTimer->stop();
        m_loginFlowActive = false;
        emit loginFlowActiveChanged();
        setBusy(false);
        setError(result.error, result.detail);
    }
}

void VaultController::cancelLoginFlow()
{
    if (!m_loginFlowActive) {
        return;
    }
    m_loginPollTimer->stop();
    m_loginFlowActive = false;
    emit loginFlowActiveChanged();
    setBusy(false);
    setError(QStringLiteral("login_cancelled"));
}

void VaultController::connectToServer(const QString &server,
                                      const QString &user,
                                      const QString &appPassword)
{
    clearError();

    const QUrl base = NcClient::apiBase(server);
    if (!base.isValid() || base.host().isEmpty()
            || (base.scheme() != QLatin1String("http")
                && base.scheme() != QLatin1String("https"))) {
        setError(QStringLiteral("invalid_url"));
        return;
    }
    if (user.trimmed().isEmpty()) {
        setError(QStringLiteral("user_required"));
        return;
    }
    if (appPassword.isEmpty()) {
        setError(QStringLiteral("password_required"));
        return;
    }

    m_restored = true;
    m_server = server.trimmed();
    m_user = user.trimmed();
    m_appPassword = appPassword;
    m_hasAccount = true;
    m_sessionId.clear();
    m_keychain = Keychain();

    SecretStore::setProtectedMode(m_fingerprintLock);
    SecretStore::save(m_server, m_user, m_appPassword);
    m_secretsActive = SecretStore::usingSecrets();
    emit secretsActiveChanged();
    emit accountChanged();
    setState(QStringLiteral("locked"));
    startSessionRequest();
}

void VaultController::startSessionRequest()
{
    if (m_server.isEmpty() || m_user.isEmpty() || m_appPassword.isEmpty()) {
        setState(QStringLiteral("setup"));
        return;
    }

    setBusy(true);
    clearError();

    const QString server = m_server;
    const QString user = m_user;
    const QString password = m_appPassword;

    runTask<SessionResult>([server, user, password]() {
        SessionResult result;

        NcClient client(NcClient::apiBase(server), user, password);
        const ApiResult response = client.get(QStringLiteral("1.0/session/request"));
        if (!response.ok) {
            result.code = friendlyApiCode(response);
            result.detail = response.detail;
            return result;
        }

        const QJsonObject root = response.json.object();
        const QJsonObject challenge = root.value(QStringLiteral("challenge")).toObject();
        const QJsonArray salts = challenge.value(QStringLiteral("salts")).toArray();

        if (salts.size() == 3) {
            bool challengeOk = false;
            result.challenge = PwdChallenge::fromJson(challenge, &challengeOk);
            result.cse = challengeOk;
            if (!challengeOk) {
                result.code = QStringLiteral("challenge_invalid");
                return result;
            }
        } else {
            result.cse = false;
        }

        result.sessionId = client.sessionId();
        result.ok = true;
        return result;
    }, [this](const SessionResult &result) {
        setBusy(false);
        if (!result.ok) {
            setError(result.code, result.detail);
            setState(QStringLiteral("setup"));
            return;
        }

        m_challenge = result.challenge;
        m_cse = result.cse;
        m_sessionId = result.sessionId;
        emit cseEnabledChanged();

        if (m_cse) {
            setState(QStringLiteral("locked"));
        } else {
            setState(QStringLiteral("unlocked"));
            startLoad();
        }
    });
}

void VaultController::unlock(const QString &masterPassword)
{
    if (m_state != QLatin1String("locked")) {
        return;
    }
    if (masterPassword.size() < 12) {
        setError(QStringLiteral("master_length"));
        return;
    }
    if (masterPassword.size() > 128) {
        setError(QStringLiteral("master_length"));
        return;
    }

    clearError();
    setBusy(true);

    const QString master = masterPassword;
    const QString server = m_server;
    const QString user = m_user;
    const QString password = m_appPassword;
    const PwdChallenge challenge = m_challenge;

    runTask<UnlockResult>([server, user, password, master, challenge]() {
        UnlockResult result;

        const QString solved = challenge.solve(master);
        if (solved.isEmpty()) {
            result.code = QStringLiteral("master");
            return result;
        }

        NcClient client(NcClient::apiBase(server), user, password);
        QJsonObject body;
        body.insert(QStringLiteral("challenge"), solved);
        const ApiResult response = client.post(QStringLiteral("1.0/session/open"), body);

        if (!response.ok) {
            if (response.status == 401) {
                result.code = QStringLiteral("master");
            } else if (response.status == 403) {
                result.code = QStringLiteral("too_many_attempts");
            } else {
                result.code = friendlyApiCode(response);
                result.detail = response.detail;
            }
            return result;
        }

        const QString keychainHex = response.json.object()
                .value(QStringLiteral("keys")).toObject()
                .value(QStringLiteral("CSEv1r1")).toString();
        if (keychainHex.isEmpty()) {
            result.code = QStringLiteral("no_keychain");
            return result;
        }

        bool keychainOk = false;
        result.keychain = Keychain::fromEncryptedJson(keychainHex, master, &keychainOk);
        if (!keychainOk) {
            result.code = QStringLiteral("master");
            return result;
        }

        result.sessionId = client.sessionId();
        result.ok = true;
        return result;
    }, [this](const UnlockResult &result) {
        setBusy(false);
        if (!result.ok) {
            setError(result.code, result.detail);
            return;
        }

        m_sessionId = result.sessionId;
        m_keychain = result.keychain;
        setState(QStringLiteral("unlocked"));
        startLoad();
    });
}

void VaultController::reconnect()
{
    if (m_state != QLatin1String("locked") || m_appPassword.isEmpty()) {
        return;
    }
    startSessionRequest();
}

void VaultController::authenticateWithFingerprint()
{
    // Kept for QML API compatibility. Device authentication is performed by
    // the org.nemomobile.devicelock Authenticator in FingerprintPage.qml.
    setError(QStringLiteral("fingerprint_failed"),
             QStringLiteral("Use the device authentication UI"));
}

void VaultController::completeFingerprintAuth()
{
    if (!m_fingerprintRequired && m_state != QLatin1String("locked")) {
        return;
    }

    if (m_appPassword.isEmpty()) {
        QString server;
        QString user;
        QString password;
        if (SecretStore::load(&server, &user, &password)) {
            m_appPassword = password;
        }
    }
    if (m_appPassword.isEmpty()) {
        setError(QStringLiteral("fingerprint_failed"),
                 QStringLiteral("app password unavailable"));
        return;
    }

    clearError();
    m_fingerprintRequired = false;
    emit fingerprintRequiredChanged();
    startSessionRequest();
}

void VaultController::fingerprintAuthFailed(const QString &detail)
{
    logFingerprint(QStringLiteral("device auth failed: %1").arg(detail));
    setError(QStringLiteral("fingerprint_failed"), detail);
}

void VaultController::setFingerprintLock(bool enabled)
{
    if (enabled == m_fingerprintLock) {
        return;
    }

    if (enabled) {
        setError(QStringLiteral("fingerprint_unsupported"));
        emit fingerprintLockChanged();
        return;
    }

    clearError();
    setBusy(true);
    logFingerprint(QStringLiteral("set lock enabled=%1").arg(enabled ? 1 : 0));

    QString error;
    if (enabled) {
        if (m_appPassword.isEmpty()) {
            setBusy(false);
            setError(QStringLiteral("fingerprint_needs_unlock"));
            emit fingerprintLockChanged();
            return;
        }
        SecretStore::setProtectedMode(false);
        if (!SecretStore::storeAppPassword(m_appPassword)) {
            setBusy(false);
            setError(QStringLiteral("fingerprint_enable_failed"));
            emit fingerprintLockChanged();
            return;
        }
        SecretStore::deleteProtected();
        m_secretsActive = SecretStore::usingSecrets();
        emit secretsActiveChanged();
        m_fingerprintLock = true;
        logFingerprint(QStringLiteral("enable ok"));
        QSettings(settingsPath(), QSettings::IniFormat).setValue(QStringLiteral("fingerprintLock"), true);
    } else {
        SecretStore::setProtectedMode(false);
        if (!m_appPassword.isEmpty()) {
            SecretStore::storeAppPassword(m_appPassword);
        }
        SecretStore::deleteProtected();
        m_fingerprintLock = false;
        logFingerprint(QStringLiteral("disable ok"));
        QSettings(settingsPath(), QSettings::IniFormat).setValue(QStringLiteral("fingerprintLock"), false);
    }

    setBusy(false);
    emit fingerprintLockChanged();
    emit toast(m_fingerprintLock ? QStringLiteral("fingerprint_enabled")
                                 : QStringLiteral("fingerprint_disabled"));
}

void VaultController::setViewMode(const QString &mode)
{
    if (mode != QLatin1String("list") && mode != QLatin1String("folders")) {
        return;
    }
    if (m_viewMode == mode) {
        return;
    }
    m_viewMode = mode;
    QSettings(settingsPath(), QSettings::IniFormat)
            .setValue(QStringLiteral("viewMode"), mode);

    m_folderFilter.clear();
    m_passwords->setFolderFilter(QString());
    emit filtersChanged();
    emit viewModeChanged();
}

void VaultController::setPasswordStrength(const QString &strength)
{
    if (strength != QLatin1String("low")
            && strength != QLatin1String("medium")
            && strength != QLatin1String("high")
            && strength != QLatin1String("veryhigh")) {
        return;
    }
    if (m_passwordStrength == strength) {
        return;
    }
    m_passwordStrength = strength;
    QSettings(settingsPath(), QSettings::IniFormat)
            .setValue(QStringLiteral("passwordStrength"), strength);
    emit passwordStrengthChanged();
}

void VaultController::setLanguage(const QString &language)
{
    if (language.isEmpty() || m_language == language) {
        return;
    }
    m_language = language;
    QSettings(settingsPath(), QSettings::IniFormat)
            .setValue(QStringLiteral("language"), language);
    emit languageChanged();
}

bool VaultController::pinMatches(const QString &pin) const
{
    if (m_pinSalt.size() != 16 || m_pinHash.size() != 32) {
        return false;
    }
    const QByteArray hash = Sodium::pwhash(pin.toUtf8(), m_pinSalt, 32);
    return !hash.isEmpty() && Sodium::constantEquals(hash, m_pinHash);
}

void VaultController::storePin(const QString &pin)
{
    m_pinSalt = Sodium::randomBytes(16);
    m_pinHash = Sodium::pwhash(pin.toUtf8(), m_pinSalt, 32);

    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("pinEnabled"), true);
    settings.setValue(QStringLiteral("pinSalt"), Sodium::toHex(m_pinSalt));
    settings.setValue(QStringLiteral("pinHash"), Sodium::toHex(m_pinHash));
}

void VaultController::verifyPin(const QString &pin)
{
    if (!m_pinEnabled || !m_pinRequired) {
        return;
    }

    clearError();
    const qint64 now = QDateTime::currentMSecsSinceEpoch() / 1000;
    if (m_pinLockedUntil > now) {
        setError(QStringLiteral("pin_locked"));
        return;
    }
    if (!validPin(pin)) {
        setError(QStringLiteral("pin_incorrect"));
        return;
    }

    setBusy(true);
    const bool ok = pinMatches(pin);
    setBusy(false);

    if (!ok) {
        ++m_pinFailures;
        if (m_pinFailures >= 5) {
            m_pinLockedUntil = now + 30;
            m_pinFailures = 0;
            setError(QStringLiteral("pin_locked"));
        } else {
            setError(QStringLiteral("pin_incorrect"));
        }
        return;
    }

    m_pinFailures = 0;
    m_pinLockedUntil = 0;
    m_pinRequired = false;
    emit pinRequiredChanged();
    startSessionRequest();
}

void VaultController::enablePin(const QString &pin)
{
    if (!validPin(pin)) {
        setError(QStringLiteral("pin_invalid"));
        return;
    }
    clearError();
    storePin(pin);
    m_pinEnabled = true;
    m_pinRequired = false;
    emit pinEnabledChanged();
    emit pinRequiredChanged();
    emit toast(QStringLiteral("pin_enabled"));
}

void VaultController::changePin(const QString &currentPin, const QString &newPin)
{
    if (!m_pinEnabled) {
        return;
    }
    if (!pinMatches(currentPin)) {
        setError(QStringLiteral("pin_incorrect"));
        return;
    }
    if (!validPin(newPin)) {
        setError(QStringLiteral("pin_invalid"));
        return;
    }
    clearError();
    storePin(newPin);
    emit toast(QStringLiteral("pin_changed"));
}

void VaultController::disablePin(const QString &currentPin)
{
    if (!m_pinEnabled) {
        return;
    }
    if (!pinMatches(currentPin)) {
        setError(QStringLiteral("pin_incorrect"));
        return;
    }
    clearError();
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue(QStringLiteral("pinEnabled"), false);
    settings.remove(QStringLiteral("pinSalt"));
    settings.remove(QStringLiteral("pinHash"));
    m_pinEnabled = false;
    m_pinRequired = false;
    emit pinEnabledChanged();
    emit pinRequiredChanged();
    emit toast(QStringLiteral("pin_disabled"));
}

void VaultController::lock()
{
    if (m_pinEnabled) {
        m_pinRequired = true;
        emit pinRequiredChanged();
    } else if (m_fingerprintLock) {
        m_fingerprintRequired = true;
        emit fingerprintRequiredChanged();
    }

    m_keychain = Keychain();
    m_sessionId.clear();

    m_passwords->setEntries(QVector<PasswordEntry>());
    m_trash->setEntries(QVector<PasswordEntry>());
    m_folderPasswords->setEntries(QVector<PasswordEntry>());
    m_openFolderId.clear();
    m_folders.clear();
    m_tags.clear();
    m_folderLabels.clear();
    m_tagLabels.clear();
    m_totpServices.clear();
    m_totpIndex = 0;

    m_lockTimer->stop();
    stopKeepAlive();

    emit foldersChanged();
    emit tagsChanged();
    emit totpChanged();
    setState(m_hasAccount ? QStringLiteral("locked") : QStringLiteral("setup"));
}

void VaultController::wipeAccount()
{
    SecretStore::clear();
    SecretStore::setProtectedMode(false);
    m_secretsActive = false;
    m_fingerprintLock = false;
    m_fingerprintRequired = false;
    m_pinEnabled = false;
    m_pinRequired = false;
    QSettings accountSettings(settingsPath(), QSettings::IniFormat);
    accountSettings.setValue(QStringLiteral("fingerprintLock"), false);
    accountSettings.setValue(QStringLiteral("pinEnabled"), false);
    accountSettings.remove(QStringLiteral("pinSalt"));
    accountSettings.remove(QStringLiteral("pinHash"));
    emit secretsActiveChanged();
    emit fingerprintLockChanged();
    emit fingerprintRequiredChanged();
    emit pinEnabledChanged();
    emit pinRequiredChanged();
    m_hasAccount = false;
    m_server.clear();
    m_user.clear();
    m_appPassword.clear();
    m_challenge = PwdChallenge();
    m_cse = false;
    m_restored = true;
    lock();
    emit accountChanged();
}

void VaultController::openFolder(const QString &folderId)
{
    m_openFolderId = folderId;
    m_folderPasswords->setSearch(QString());
    m_folderPasswords->setTagFilter(QString());
    m_folderPasswords->setEntries(m_passwords->entries());
    m_folderPasswords->setFolderFilter(folderId);
}

void VaultController::closeFolder()
{
    m_openFolderId.clear();
    m_folderPasswords->setEntries(QVector<PasswordEntry>());
}

void VaultController::setFolderSearch(const QString &text)
{
    logDebug(QStringLiteral("setFolderSearch '%1'").arg(text));
    m_folderPasswords->setSearch(text);
}

void VaultController::refresh()
{
    if (m_state == QLatin1String("unlocked")) {
        startLoad();
    }
}

void VaultController::reloadTrash()
{
    refresh();
}

void VaultController::setSearch(const QString &text)
{
    logDebug(QStringLiteral("setSearch '%1'").arg(text));
    m_search = text;
    m_passwords->setSearch(text);
    emit filtersChanged();
}

void VaultController::setFolderFilter(const QString &folderId)
{
    m_folderFilter = folderId;
    m_passwords->setFolderFilter(folderId);
    emit filtersChanged();
}

void VaultController::setTagFilter(const QString &tagId)
{
    m_tagFilter = tagId;
    m_passwords->setTagFilter(tagId);
    emit filtersChanged();
}

void VaultController::clearFilters()
{
    m_search.clear();
    m_folderFilter.clear();
    m_tagFilter.clear();
    m_passwords->setSearch(QString());
    m_passwords->setFolderFilter(QString());
    m_passwords->setTagFilter(QString());
    emit filtersChanged();
}

QVariantMap VaultController::password(const QString &id) const
{
    PasswordEntry entry = m_passwords->entryById(id);
    if (entry.id.isEmpty()) {
        entry = m_trash->entryById(id);
    }

    QVariantMap map;
    if (entry.id.isEmpty()) {
        return map;
    }

    map.insert(QStringLiteral("id"), entry.id);
    map.insert(QStringLiteral("revision"), entry.revision);
    map.insert(QStringLiteral("label"), entry.label);
    map.insert(QStringLiteral("username"), entry.username);
    map.insert(QStringLiteral("password"), entry.password);
    map.insert(QStringLiteral("url"), entry.url);
    map.insert(QStringLiteral("notes"), entry.notes);
    map.insert(QStringLiteral("folderId"), entry.folderId);
    map.insert(QStringLiteral("folderLabel"), m_folderLabels.value(entry.folderId));
    map.insert(QStringLiteral("favorite"), entry.favorite);
    map.insert(QStringLiteral("hidden"), entry.hidden);
    map.insert(QStringLiteral("trashed"), entry.trashed);
    map.insert(QStringLiteral("status"), entry.status);
    map.insert(QStringLiteral("statusCode"), entry.statusCode);
    map.insert(QStringLiteral("tagIds"), entry.tagIds);
    map.insert(QStringLiteral("customFields"), customFieldsToVariant(entry.customFields));
    map.insert(QStringLiteral("hasPassword"), !entry.password.isEmpty());
    return map;
}

void VaultController::createPassword(const QVariantMap &data)
{
    if (m_state != QLatin1String("unlocked")) {
        return;
    }
    clearError();

    PasswordEntry entry = entryFromMap(data);
    if (entry.label.trimmed().isEmpty()) {
        setError(QStringLiteral("label_required"));
        return;
    }

    runOperation(QStringLiteral("POST"), QStringLiteral("1.0/password/create"),
                 passwordPayload(entry, m_keychain), QStringLiteral("created"));
}

void VaultController::updatePassword(const QVariantMap &data)
{
    if (m_state != QLatin1String("unlocked")) {
        return;
    }
    clearError();

    const QString id = data.value(QStringLiteral("id")).toString();
    PasswordEntry existing = m_passwords->entryById(id);
    if (existing.id.isEmpty()) {
        existing = m_trash->entryById(id);
    }
    if (existing.id.isEmpty()) {
        setError(QStringLiteral("not_found"));
        return;
    }

    PasswordEntry entry = entryFromMap(data);
    entry.id = existing.id;
    entry.revision = existing.revision;
    if (entry.folderId.isEmpty()) {
        entry.folderId = existing.folderId;
    }
    if (!data.contains(QStringLiteral("customFields"))) {
        entry.customFields = existing.customFields;
        entry.customFieldsJson = existing.customFieldsJson;
    }
    if (!data.contains(QStringLiteral("tagIds"))) {
        entry.tagIds = existing.tagIds;
    }
    if (entry.label.trimmed().isEmpty()) {
        setError(QStringLiteral("label_required"));
        return;
    }

    runOperation(QStringLiteral("PATCH"), QStringLiteral("1.0/password/update"),
                 passwordPayload(entry, m_keychain), QStringLiteral("updated"));
}

void VaultController::deletePassword(const QString &id)
{
    if (m_state != QLatin1String("unlocked")) {
        return;
    }
    clearError();

    PasswordEntry entry = m_passwords->entryById(id);
    if (entry.id.isEmpty()) {
        entry = m_trash->entryById(id);
    }
    if (entry.id.isEmpty()) {
        setError(QStringLiteral("not_found"));
        return;
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("id"), entry.id);
    payload.insert(QStringLiteral("revision"), entry.revision);
    runOperation(QStringLiteral("DELETE"), QStringLiteral("1.0/password/delete"),
                 payload, QStringLiteral("deleted"));
}

void VaultController::restorePassword(const QString &id)
{
    if (m_state != QLatin1String("unlocked")) {
        return;
    }
    clearError();

    const PasswordEntry entry = m_trash->entryById(id);
    if (entry.id.isEmpty()) {
        setError(QStringLiteral("not_found"));
        return;
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("id"), entry.id);
    payload.insert(QStringLiteral("revision"), entry.revision);
    runOperation(QStringLiteral("PATCH"), QStringLiteral("1.0/password/restore"),
                 payload, QStringLiteral("restored"));
}

void VaultController::createFolder(const QString &label, const QString &parentId)
{
    if (m_state != QLatin1String("unlocked")) {
        return;
    }
    if (label.trimmed().isEmpty()) {
        setError(QStringLiteral("label_required"));
        return;
    }
    clearError();

    Folder folder;
    folder.label = label.trimmed();
    folder.parentId = parentId.isEmpty() ? defaultFolderId() : parentId;
    runOperation(QStringLiteral("POST"), QStringLiteral("1.0/folder/create"),
                 folderPayload(folder, m_keychain), QStringLiteral("folder_created"));
}

void VaultController::updateFolder(const QString &id,
                                   const QString &label,
                                   const QString &parentId)
{
    if (m_state != QLatin1String("unlocked")) {
        return;
    }
    const Folder existing = folderById(id);
    if (existing.id.isEmpty()) {
        setError(QStringLiteral("not_found"));
        return;
    }
    if (label.trimmed().isEmpty()) {
        setError(QStringLiteral("label_required"));
        return;
    }
    clearError();

    Folder folder = existing;
    folder.label = label.trimmed();
    folder.parentId = parentId.isEmpty() ? existing.parentId : parentId;
    runOperation(QStringLiteral("PATCH"), QStringLiteral("1.0/folder/update"),
                 folderPayload(folder, m_keychain), QStringLiteral("folder_updated"));
}

void VaultController::deleteFolder(const QString &id)
{
    if (m_state != QLatin1String("unlocked")) {
        return;
    }
    const Folder folder = folderById(id);
    if (folder.id.isEmpty()) {
        setError(QStringLiteral("not_found"));
        return;
    }
    clearError();

    QJsonObject payload;
    payload.insert(QStringLiteral("id"), folder.id);
    payload.insert(QStringLiteral("revision"), folder.revision);
    runOperation(QStringLiteral("DELETE"), QStringLiteral("1.0/folder/delete"),
                 payload, QStringLiteral("folder_deleted"));
}

void VaultController::createTag(const QString &label, const QString &color)
{
    if (m_state != QLatin1String("unlocked")) {
        return;
    }
    if (label.trimmed().isEmpty()) {
        setError(QStringLiteral("label_required"));
        return;
    }
    clearError();

    Tag tag;
    tag.label = label.trimmed();
    tag.color = color.isEmpty() ? QStringLiteral("#888888") : color;
    runOperation(QStringLiteral("POST"), QStringLiteral("1.0/tag/create"),
                 tagPayload(tag, m_keychain), QStringLiteral("tag_created"));
}

void VaultController::updateTag(const QString &id,
                                const QString &label,
                                const QString &color)
{
    if (m_state != QLatin1String("unlocked")) {
        return;
    }
    const Tag existing = tagById(id);
    if (existing.id.isEmpty()) {
        setError(QStringLiteral("not_found"));
        return;
    }
    if (label.trimmed().isEmpty()) {
        setError(QStringLiteral("label_required"));
        return;
    }
    clearError();

    Tag tag = existing;
    tag.label = label.trimmed();
    tag.color = color.isEmpty() ? existing.color : color;
    runOperation(QStringLiteral("PATCH"), QStringLiteral("1.0/tag/update"),
                 tagPayload(tag, m_keychain), QStringLiteral("tag_updated"));
}

void VaultController::deleteTag(const QString &id)
{
    if (m_state != QLatin1String("unlocked")) {
        return;
    }
    const Tag tag = tagById(id);
    if (tag.id.isEmpty()) {
        setError(QStringLiteral("not_found"));
        return;
    }
    clearError();

    QJsonObject payload;
    payload.insert(QStringLiteral("id"), tag.id);
    payload.insert(QStringLiteral("revision"), tag.revision);
    runOperation(QStringLiteral("DELETE"), QStringLiteral("1.0/tag/delete"),
                 payload, QStringLiteral("tag_deleted"));
}

void VaultController::copySecret(const QString &text)
{
    if (text.isEmpty()) {
        return;
    }
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);

    if (m_clipboardSeconds > 0) {
        QTimer::singleShot(m_clipboardSeconds * 1000, this, [text]() {
            QClipboard *cb = QGuiApplication::clipboard();
            if (cb->text() == text) {
                cb->clear();
            }
        });
    }
}

QString VaultController::generatePasswordForStrength()
{
    if (m_passwordStrength == QLatin1String("low")) {
        return generatePassword(12, true, false);
    }
    if (m_passwordStrength == QLatin1String("medium")) {
        return generatePassword(16, true, false);
    }
    if (m_passwordStrength == QLatin1String("veryhigh")) {
        return generatePassword(32, true, true);
    }
    return generatePassword(20, true, true);
}

QString VaultController::generatePassword(int length, bool digits, bool symbols)
{
    if (length < 8) {
        length = 20;
    }
    if (length > 128) {
        length = 128;
    }

    const QString letters = QStringLiteral("abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ");
    const QString digitChars = QStringLiteral("23456789");
    const QString symbolChars = QStringLiteral("!@#$%^&*()-_=+[]{};:,.?");

    QString pool = letters;
    if (digits) {
        pool += digitChars;
    }
    if (symbols) {
        pool += symbolChars;
    }

    QString password = randomChars(pool, length);
    if (password.isEmpty()) {
        return QString();
    }

    const QByteArray extra = Sodium::randomBytes(8);
    int position = 0;
    if (digits && !containsAny(password, digitChars)) {
        position = static_cast<unsigned char>(extra.at(0)) % password.size();
        password[position] = digitChars.at(static_cast<unsigned char>(extra.at(1)) % digitChars.size());
    }
    if (symbols && !containsAny(password, symbolChars)) {
        position = static_cast<unsigned char>(extra.at(2)) % password.size();
        password[position] = symbolChars.at(static_cast<unsigned char>(extra.at(3)) % symbolChars.size());
    }
    return password;
}

void VaultController::noteActivity()
{
    if (m_state == QLatin1String("unlocked") && m_lockSeconds > 0) {
        m_lockTimer->start(m_lockSeconds * 1000);
    }
}

QString VaultController::folderLabel(const QString &id) const
{
    return m_folderLabels.value(id);
}

QString VaultController::tagLabel(const QString &id) const
{
    return m_tagLabels.value(id);
}

QVariantMap VaultController::currentTotp() const
{
    QVariantMap map;
    if (m_totpServices.isEmpty()) {
        return map;
    }

    const TotpService &service = m_totpServices.at(
                qBound(0, m_totpIndex, m_totpServices.size() - 1));
    const qint64 now = QDateTime::currentMSecsSinceEpoch() / 1000;
    const QString code = Totp::code(service.config, now);

    map.insert(QStringLiteral("passwordId"), service.passwordId);
    map.insert(QStringLiteral("label"), service.entryLabel);
    map.insert(QStringLiteral("issuer"), service.config.issuer);
    map.insert(QStringLiteral("account"), service.config.accountName);
    map.insert(QStringLiteral("code"), code);
    map.insert(QStringLiteral("formattedCode"),
               Totp::formattedCode(code, service.config.digits));
    map.insert(QStringLiteral("remaining"), Totp::remaining(service.config, now));
    map.insert(QStringLiteral("period"), service.config.period);
    map.insert(QStringLiteral("digits"), service.config.digits);
    map.insert(QStringLiteral("index"), m_totpIndex);
    map.insert(QStringLiteral("count"), m_totpServices.size());
    return map;
}

QVariantMap VaultController::totpForPassword(const QString &id) const
{
    for (const TotpService &service : m_totpServices) {
        if (service.passwordId != id) {
            continue;
        }
        QVariantMap map;
        const qint64 now = QDateTime::currentMSecsSinceEpoch() / 1000;
        const QString code = Totp::code(service.config, now);
        map.insert(QStringLiteral("label"), service.entryLabel);
        map.insert(QStringLiteral("issuer"), service.config.issuer);
        map.insert(QStringLiteral("account"), service.config.accountName);
        map.insert(QStringLiteral("code"), code);
        map.insert(QStringLiteral("formattedCode"),
                   Totp::formattedCode(code, service.config.digits));
        map.insert(QStringLiteral("remaining"), Totp::remaining(service.config, now));
        map.insert(QStringLiteral("period"), service.config.period);
        map.insert(QStringLiteral("digits"), service.config.digits);
        return map;
    }
    return QVariantMap();
}

bool VaultController::hasTotp(const QString &id) const
{
    for (const TotpService &service : m_totpServices) {
        if (service.passwordId == id) {
            return true;
        }
    }
    return false;
}

void VaultController::nextTotp()
{
    if (m_totpServices.isEmpty()) {
        return;
    }
    m_totpIndex = (m_totpIndex + 1) % m_totpServices.size();
    emit totpChanged();
}

void VaultController::previousTotp()
{
    if (m_totpServices.isEmpty()) {
        return;
    }
    m_totpIndex = (m_totpIndex - 1 + m_totpServices.size()) % m_totpServices.size();
    emit totpChanged();
}

void VaultController::setTotpIndex(int index)
{
    if (m_totpServices.isEmpty()) {
        m_totpIndex = 0;
        return;
    }
    const int bounded = qBound(0, index, m_totpServices.size() - 1);
    if (bounded == m_totpIndex) {
        return;
    }
    m_totpIndex = bounded;
    emit totpChanged();
}

void VaultController::rebuildTotpServices()
{
    QString currentId;
    if (!m_totpServices.isEmpty() && m_totpIndex >= 0 && m_totpIndex < m_totpServices.size()) {
        currentId = m_totpServices.at(m_totpIndex).passwordId;
    }

    m_totpServices.clear();

    for (const PasswordEntry &entry : m_passwords->entries()) {
        for (const CustomField &field : entry.customFields) {
            const QString label = field.label.trimmed();
            const QString value = field.value.trimmed();
            bool candidate = label.compare(QStringLiteral("client.ios.otp"),
                                           Qt::CaseInsensitive) == 0;
            if (!candidate && label.contains(QStringLiteral("otp"), Qt::CaseInsensitive)) {
                candidate = value.startsWith(QStringLiteral("otpauth://"), Qt::CaseInsensitive)
                        || value.contains(QStringLiteral("\"secret\""));
            }
            if (!candidate) {
                continue;
            }
            const TotpConfig config = Totp::fromValue(value);
            if (!config.valid) {
                continue;
            }
            TotpService service;
            service.passwordId = entry.id;
            service.entryLabel = entry.label;
            service.config = config;
            m_totpServices.append(service);
            break;
        }
    }

    int newIndex = 0;
    if (!currentId.isEmpty()) {
        for (int i = 0; i < m_totpServices.size(); ++i) {
            if (m_totpServices.at(i).passwordId == currentId) {
                newIndex = i;
                break;
            }
        }
    }
    m_totpIndex = m_totpServices.isEmpty()
            ? 0
            : qBound(0, newIndex, m_totpServices.size() - 1);

    emit totpChanged();
}

void VaultController::logDebug(const QString &text)
{
    QFile file(SecretStore::directoryPath() + QStringLiteral("/debug.log"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        file.write(QDateTime::currentDateTime().toString(Qt::ISODate).toUtf8());
        file.write(" ");
        file.write(text.toUtf8());
        file.write("\n");
    }
}

void VaultController::clearError()
{
    if (m_errorCode.isEmpty() && m_errorDetail.isEmpty()) {
        return;
    }
    m_errorCode.clear();
    m_errorDetail.clear();
    emit errorCodeChanged();
}

void VaultController::handleApplicationState(int state)
{
    // Keep the vault unlocked while the app is in the cover so that the TOTP
    // codes can be consulted. The inactivity timer still locks it.
    if (state == Qt::ApplicationSuspended) {
        noteActivity();
    }
}

void VaultController::setState(const QString &state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();

    if (state == QLatin1String("unlocked")) {
        noteActivity();
        if (!m_sessionId.isEmpty()) {
            startKeepAlive();
        }
    } else {
        m_lockTimer->stop();
        stopKeepAlive();
    }
}

void VaultController::setBusy(bool busy)
{
    if (m_busy == busy) {
        return;
    }
    m_busy = busy;
    emit busyChanged();
}

void VaultController::setError(const QString &code, const QString &detail)
{
    m_errorCode = code;
    m_errorDetail = detail;
    emit errorCodeChanged();
}

void VaultController::startLoad()
{
    if (m_state != QLatin1String("unlocked")) {
        return;
    }

    setBusy(true);
    clearError();
    logDebug(QStringLiteral("startLoad"));

    const QString server = m_server;
    const QString user = m_user;
    const QString password = m_appPassword;
    const QString sessionId = m_sessionId;
    const Keychain keychain = m_keychain;
    const bool cse = m_cse;

    runTask<LoadResult>([server, user, password, sessionId, keychain, cse]() {
        LoadResult result;

        NcClient client(NcClient::apiBase(server), user, password, sessionId);

        QJsonObject details;
        details.insert(QStringLiteral("details"),
                       QStringLiteral("model+folder+tags"));
        const ApiResult passwords = client.post(QStringLiteral("1.0/password/list"), details);
        result.debug = QStringLiteral("password/list status=%1").arg(passwords.status);
        if (!passwords.ok) {
            result.code = friendlyApiCode(passwords);
            result.detail = passwords.detail;
            return result;
        }

        QJsonObject trashCriteria;
        trashCriteria.insert(QStringLiteral("trashed"), true);
        QJsonObject trashRequest;
        trashRequest.insert(QStringLiteral("criteria"), trashCriteria);
        const ApiResult trash = client.post(QStringLiteral("1.0/password/find"), trashRequest);

        const ApiResult folders = client.post(QStringLiteral("1.0/folder/list"));
        if (!folders.ok) {
            result.code = friendlyApiCode(folders);
            result.detail = folders.detail;
            return result;
        }

        const ApiResult tags = client.post(QStringLiteral("1.0/tag/list"));

        QString decryptError;
        for (const QJsonValue &value : passwords.json.array()) {
            PasswordEntry entry = PasswordEntry::fromJson(value.toObject());
            if (cse && !decryptPassword(entry, keychain, &decryptError)) {
                result.code = QStringLiteral("decrypt_failed");
                result.detail = decryptError;
                return result;
            }
            result.passwords.append(entry);
        }

        if (trash.ok) {
            for (const QJsonValue &value : trash.json.array()) {
                PasswordEntry entry = PasswordEntry::fromJson(value.toObject());
                if (cse && !decryptPassword(entry, keychain, &decryptError)) {
                    result.code = QStringLiteral("decrypt_failed");
                    result.detail = decryptError;
                    return result;
                }
                result.trash.append(entry);
            }
        }

        for (const QJsonValue &value : folders.json.array()) {
            Folder folder = Folder::fromJson(value.toObject());
            if (cse && !decryptFolder(folder, keychain, &decryptError)) {
                result.code = QStringLiteral("decrypt_failed");
                result.detail = decryptError;
                return result;
            }
            result.folders.append(folder);
        }

        if (tags.ok) {
            for (const QJsonValue &value : tags.json.array()) {
                Tag tag = Tag::fromJson(value.toObject());
                if (cse && !decryptTag(tag, keychain, &decryptError)) {
                    result.code = QStringLiteral("decrypt_failed");
                    result.detail = decryptError;
                    return result;
                }
                result.tags.append(tag);
            }
        }

        result.debug += QStringLiteral(" passwords=%1 trash=%2 folders=%3 tags=%4")
                .arg(result.passwords.size()).arg(result.trash.size())
                .arg(result.folders.size()).arg(result.tags.size());
        result.ok = true;
        return result;
    }, [this](const LoadResult &result) {
        setBusy(false);
        logDebug(QStringLiteral("load result ok=%1 %2").arg(result.ok ? 1 : 0).arg(result.debug));
        if (!result.ok) {
            if (result.code == QLatin1String("auth")) {
                lock();
                setError(QStringLiteral("session_expired"));
            } else {
                setError(result.code, result.detail);
            }
            return;
        }
        applyLoaded(result);
    });
}

void VaultController::applyLoaded(const LoadResult &result)
{
    logDebug(QStringLiteral("applyLoaded start"));
    m_passwords->setEntries(result.passwords);
    m_trash->setEntries(result.trash);

    m_folders = result.folders;
    m_tags = result.tags;
    rebuildFolderLabels();

    m_passwords->setFolderLabels(m_folderLabels);
    m_trash->setFolderLabels(m_folderLabels);

    m_passwords->setSearch(m_search);
    m_passwords->setFolderFilter(m_folderFilter);
    m_passwords->setTagFilter(m_tagFilter);

    logDebug(QStringLiteral("setEntries done"));
    rebuildTotpServices();
    logDebug(QStringLiteral("totp rebuilt count=%1").arg(m_totpServices.size()));

    if (!m_openFolderId.isEmpty()) {
        openFolder(m_openFolderId);
    }

    emit foldersChanged();
    emit tagsChanged();
    logDebug(QStringLiteral("applyLoaded done"));
}

void VaultController::rebuildFolderLabels()
{
    m_folderLabels.clear();
    for (const Folder &folder : m_folders) {
        m_folderLabels.insert(folder.id, folder.label);
    }

    m_tagLabels.clear();
    for (const Tag &tag : m_tags) {
        m_tagLabels.insert(tag.id, tag.label);
    }
}

void VaultController::startKeepAlive()
{
    if (m_sessionId.isEmpty()) {
        return;
    }
    m_keepAliveTimer->start();
}

void VaultController::stopKeepAlive()
{
    m_keepAliveTimer->stop();
}

void VaultController::keepAlive()
{
    if (m_state != QLatin1String("unlocked") || m_sessionId.isEmpty()) {
        return;
    }

    const QString server = m_server;
    const QString user = m_user;
    const QString password = m_appPassword;
    const QString sessionId = m_sessionId;

    runTask<OpResult>([server, user, password, sessionId]() {
        OpResult result;
        NcClient client(NcClient::apiBase(server), user, password, sessionId);
        const ApiResult response = client.get(QStringLiteral("1.0/session/keepalive"));
        result.ok = response.ok;
        result.status = response.status;
        result.code = friendlyApiCode(response);
        result.detail = response.detail;
        return result;
    }, [this](const OpResult &result) {
        if (!result.ok && result.code == QLatin1String("auth")) {
            lock();
            setError(QStringLiteral("session_expired"));
        }
    });
}

void VaultController::runOperation(const QString &verb,
                                   const QString &path,
                                   const QJsonObject &payload,
                                   const QString &successKey)
{
    if (m_state != QLatin1String("unlocked")) {
        return;
    }
    setBusy(true);

    const QString server = m_server;
    const QString user = m_user;
    const QString password = m_appPassword;
    const QString sessionId = m_sessionId;

    runTask<OpResult>([server, user, password, sessionId, verb, path, payload]() {
        OpResult result;
        NcClient client(NcClient::apiBase(server), user, password, sessionId);
        ApiResult response;
        if (verb == QLatin1String("POST")) {
            response = client.post(path, payload);
        } else if (verb == QLatin1String("PATCH")) {
            response = client.patch(path, payload);
        } else {
            response = client.remove(path, payload);
        }
        result.ok = response.ok;
        result.status = response.status;
        result.code = friendlyApiCode(response);
        result.detail = response.detail;
        return result;
    }, [this, successKey](const OpResult &result) {
        setBusy(false);
        if (result.ok) {
            emit toast(successKey);
            startLoad();
            return;
        }
        if (result.code == QLatin1String("auth")) {
            lock();
            setError(QStringLiteral("session_expired"));
        } else {
            setError(result.code, result.detail);
        }
    });
}

PasswordEntry VaultController::entryFromMap(const QVariantMap &data) const
{
    PasswordEntry entry;
    entry.id = data.value(QStringLiteral("id")).toString();
    entry.revision = data.value(QStringLiteral("revision")).toString();
    entry.label = data.value(QStringLiteral("label")).toString();
    entry.username = data.value(QStringLiteral("username")).toString();
    entry.password = data.value(QStringLiteral("password")).toString();
    entry.url = data.value(QStringLiteral("url")).toString();
    entry.notes = data.value(QStringLiteral("notes")).toString();
    entry.folderId = data.value(QStringLiteral("folderId"), defaultFolderId()).toString();
    entry.favorite = data.value(QStringLiteral("favorite"), false).toBool();
    entry.hidden = data.value(QStringLiteral("hidden"), false).toBool();
    entry.customFields = customFieldsFromVariant(data.value(QStringLiteral("customFields")));
    entry.customFieldsJson = data.value(QStringLiteral("customFieldsJson")).toString();
    const QVariant tagIdsValue = data.value(QStringLiteral("tagIds"));
    if (tagIdsValue.canConvert<QVariantList>()) {
        const QVariantList tagIds = tagIdsValue.toList();
        for (const QVariant &tagId : tagIds) {
            const QString value = tagId.toString();
            if (!value.isEmpty()) {
                entry.tagIds.append(value);
            }
        }
    }
    if (entry.customFieldsJson.isEmpty() && entry.customFields.isEmpty()) {
        entry.customFieldsJson = QStringLiteral("[]");
    }
    return entry;
}

Folder VaultController::folderById(const QString &id) const
{
    for (const Folder &folder : m_folders) {
        if (folder.id == id) {
            return folder;
        }
    }
    return Folder();
}

Tag VaultController::tagById(const QString &id) const
{
    for (const Tag &tag : m_tags) {
        if (tag.id == id) {
            return tag;
        }
    }
    return Tag();
}

QList<CustomField> VaultController::customFieldsFromVariant(const QVariant &value)
{
    QList<CustomField> fields;
    if (value.type() != QVariant::List) {
        return fields;
    }
    const QVariantList list = value.toList();
    for (const QVariant &item : list) {
        const QVariantMap map = item.toMap();
        CustomField field;
        field.label = map.value(QStringLiteral("label")).toString();
        field.type = map.value(QStringLiteral("type"), QStringLiteral("text")).toString();
        field.value = map.value(QStringLiteral("value")).toString();
        if (!field.label.isEmpty()) {
            fields.append(field);
        }
    }
    return fields;
}

QVariantList VaultController::customFieldsToVariant(const QList<CustomField> &fields)
{
    QVariantList list;
    for (const CustomField &field : fields) {
        QVariantMap map;
        map.insert(QStringLiteral("label"), field.label);
        map.insert(QStringLiteral("type"), field.type);
        map.insert(QStringLiteral("value"), field.value);
        list.append(map);
    }
    return list;
}
