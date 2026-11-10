#include "secretstore.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#include <Sailfish/Secrets/collectionnamesrequest.h>
#include <Sailfish/Secrets/createcollectionrequest.h>
#include <Sailfish/Secrets/deletesecretrequest.h>
#include <Sailfish/Secrets/result.h>
#include <Sailfish/Secrets/secret.h>
#include <Sailfish/Secrets/secretmanager.h>
#include <Sailfish/Secrets/storedsecretrequest.h>
#include <Sailfish/Secrets/storesecretrequest.h>

using namespace Sailfish::Secrets;

namespace {

const char kCollectionName[] = "HarbourNcPasswordsV2";
const char kSecretName[] = "app-password";
const char kTestSecretName[] = "test-secret";
const char kProtectedSecretName[] = "app-password-protected";
const char kProtectedTestName[] = "protectedTest";

bool g_usingSecrets = false;
bool g_protectedMode = false;
QString g_lastError;

SecretManager *secretManager()
{
    static SecretManager manager;
    return &manager;
}

Secret::Identifier secretIdentifier(const char *name)
{
    return Secret::Identifier(QString::fromLatin1(name),
                              QString::fromLatin1(kCollectionName),
                              SecretManager::DefaultEncryptedStoragePluginName);
}

Secret::Identifier standaloneIdentifier(const char *name)
{
    return Secret::Identifier(QString::fromLatin1(name),
                              QString(),
                              SecretManager::DefaultStoragePluginName);
}

QString dataDirectory()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (path.isEmpty()) {
        path = QDir::homePath() + QStringLiteral("/.local/share/harbour-ncpasswords");
    }
    return path;
}

QJsonObject readLegacyObject()
{
    QFile file(dataDirectory() + QStringLiteral("/account.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    return document.isObject() ? document.object() : QJsonObject();
}

bool storeNamedSecret(const char *name, const QString &value)
{
    const auto doStore = [name, &value]() -> Result {
        Secret secret(secretIdentifier(name));
        secret.setType(Secret::TypeBlob);
        secret.setData(value.toUtf8());

        StoreSecretRequest request;
        request.setManager(secretManager());
        request.setSecretStorageType(StoreSecretRequest::CollectionSecret);
        request.setUserInteractionMode(SecretManager::SystemInteraction);
        request.setSecret(secret);
        request.startRequest();
        request.waitForFinished();
        return request.result();
    };

    const Result first = doStore();
    if (first.code() == Result::Succeeded) {
        return true;
    }

    g_lastError = QStringLiteral("store_failed code=%1 error=%2 message=%3")
            .arg(static_cast<int>(first.code()))
            .arg(static_cast<int>(first.errorCode()))
            .arg(first.errorMessage());

    if (first.errorCode() == Result::DatabaseQueryError
            || first.errorCode() == Result::SecretAlreadyExistsError) {
        DeleteSecretRequest remove;
        remove.setManager(secretManager());
        remove.setUserInteractionMode(SecretManager::SystemInteraction);
        remove.setIdentifier(secretIdentifier(name));
        remove.startRequest();
        remove.waitForFinished();

        return doStore().code() == Result::Succeeded;
    }

    return false;
}

bool readNamedSecret(const char *name, QString *value)
{
    StoredSecretRequest request;
    request.setManager(secretManager());
    request.setUserInteractionMode(SecretManager::SystemInteraction);
    request.setIdentifier(secretIdentifier(name));
    request.startRequest();
    request.waitForFinished();

    if (request.result().code() != Result::Succeeded) {
        g_lastError = QStringLiteral("read_failed code=%1 error=%2 message=%3")
                .arg(static_cast<int>(request.result().code()))
                .arg(static_cast<int>(request.result().errorCode()))
                .arg(request.result().errorMessage());
        return false;
    }
    if (value) {
        *value = QString::fromUtf8(request.secret().data());
    }
    return true;
}

bool deleteNamedSecret(const char *name)
{
    DeleteSecretRequest request;
    request.setManager(secretManager());
    request.setUserInteractionMode(SecretManager::SystemInteraction);
    request.setIdentifier(secretIdentifier(name));
    request.startRequest();
    request.waitForFinished();
    return request.result().code() == Result::Succeeded;
}

bool deleteStandalone(const char *name)
{
    DeleteSecretRequest request;
    request.setManager(secretManager());
    request.setUserInteractionMode(SecretManager::SystemInteraction);
    request.setIdentifier(standaloneIdentifier(name));
    request.startRequest();
    request.waitForFinished();
    return request.result().code() == Result::Succeeded;
}

bool storeStandalone(const char *name, const QString &value)
{
    deleteStandalone(name);

    Secret secret(standaloneIdentifier(name));
    secret.setType(Secret::TypeBlob);
    secret.setData(value.toUtf8());

    StoreSecretRequest request;
    request.setManager(secretManager());
    request.setSecretStorageType(StoreSecretRequest::StandaloneCustomLockSecret);
    request.setCustomLockUnlockSemantic(SecretManager::CustomLockAccessRelock);
    request.setAccessControlMode(SecretManager::OwnerOnlyMode);
    request.setEncryptionPluginName(SecretManager::DefaultEncryptionPluginName);
    request.setAuthenticationPluginName(SecretManager::DefaultAuthenticationPluginName);
    request.setUserInteractionMode(SecretManager::SystemInteraction);
    request.setSecret(secret);
    request.startRequest();
    request.waitForFinished();

    if (request.result().code() != Result::Succeeded) {
        g_lastError = QStringLiteral("protected_store_failed code=%1 error=%2 message=%3")
                .arg(static_cast<int>(request.result().code()))
                .arg(static_cast<int>(request.result().errorCode()))
                .arg(request.result().errorMessage());
        return false;
    }
    return true;
}

bool readStandalone(const char *name, QString *value)
{
    StoredSecretRequest request;
    request.setManager(secretManager());
    request.setUserInteractionMode(SecretManager::SystemInteraction);
    request.setIdentifier(standaloneIdentifier(name));
    request.startRequest();
    request.waitForFinished();

    if (request.result().code() != Result::Succeeded) {
        g_lastError = QStringLiteral("protected_read_failed code=%1 error=%2 message=%3")
                .arg(static_cast<int>(request.result().code()))
                .arg(static_cast<int>(request.result().errorCode()))
                .arg(request.result().errorMessage());
        return false;
    }
    if (value) {
        *value = QString::fromUtf8(request.secret().data());
    }
    return true;
}

} // namespace

QString SecretStore::directoryPath()
{
    return dataDirectory();
}

QString SecretStore::filePath()
{
    return dataDirectory() + QStringLiteral("/account.json");
}

bool SecretStore::usingSecrets()
{
    return g_usingSecrets;
}

QString SecretStore::lastError()
{
    return g_lastError;
}

bool SecretStore::ensureCollection()
{
    static int state = -1; // -1 unknown, 0 unavailable, 1 ready
    if (state == 1) {
        return true;
    }

    SecretManager *manager = secretManager();
    if (!manager->isInitialized()) {
        g_lastError = QStringLiteral("manager_not_initialized");
        state = 0;
        return false;
    }

    CollectionNamesRequest names;
    names.setManager(manager);
    names.setStoragePluginName(SecretManager::DefaultEncryptedStoragePluginName);
    names.startRequest();
    names.waitForFinished();

    if (names.result().code() == Result::Succeeded
            && names.collectionNames().contains(QString::fromLatin1(kCollectionName))) {
        state = 1;
        return true;
    }

    CreateCollectionRequest create;
    create.setManager(manager);
    create.setCollectionLockType(CreateCollectionRequest::DeviceLock);
    create.setDeviceLockUnlockSemantic(SecretManager::DeviceLockKeepUnlocked);
    create.setAccessControlMode(SecretManager::OwnerOnlyMode);
    create.setUserInteractionMode(SecretManager::SystemInteraction);
    create.setCollectionName(QString::fromLatin1(kCollectionName));
    create.setStoragePluginName(SecretManager::DefaultEncryptedStoragePluginName);
    create.setEncryptionPluginName(SecretManager::DefaultEncryptedStoragePluginName);
    create.startRequest();
    create.waitForFinished();

    const Result result = create.result();
    if (result.code() == Result::Succeeded
            || result.errorCode() == Result::CollectionAlreadyExistsError) {
        state = 1;
        return true;
    }

    g_lastError = QStringLiteral("create_failed code=%1 error=%2 message=%3")
            .arg(static_cast<int>(result.code()))
            .arg(static_cast<int>(result.errorCode()))
            .arg(result.errorMessage());

    CollectionNamesRequest after;
    after.setManager(manager);
    after.setStoragePluginName(SecretManager::DefaultEncryptedStoragePluginName);
    after.startRequest();
    after.waitForFinished();
    g_lastError += QStringLiteral(" names_code=%1 names_error=%2 names=%3")
            .arg(static_cast<int>(after.result().code()))
            .arg(static_cast<int>(after.result().errorCode()))
            .arg(after.collectionNames().join(QLatin1Char(',')));

    state = 0;
    return false;
}

bool SecretStore::writeLegacyFile(const QString &server,
                                  const QString &user,
                                  const QString &password)
{
    QDir directory;
    if (!directory.mkpath(dataDirectory())) {
        return false;
    }
    QFile::setPermissions(dataDirectory(),
                          QFileDevice::ReadOwner | QFileDevice::WriteOwner
                          | QFileDevice::ExeOwner);

    QJsonObject object;
    object.insert(QStringLiteral("server"), server);
    object.insert(QStringLiteral("user"), user);
    if (!password.isEmpty()) {
        object.insert(QStringLiteral("password"), password);
    }

    QSaveFile file(filePath());
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
    if (!file.commit()) {
        return false;
    }
    QFile::setPermissions(filePath(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}

bool SecretStore::save(const QString &server,
                       const QString &user,
                       const QString &password)
{
    bool secretStored = false;
    if (g_protectedMode) {
        secretStored = storeProtected(password);
    } else {
        secretStored = ensureCollection() && storeNamedSecret(kSecretName, password);
    }
    g_usingSecrets = secretStored;

    // Keep the password in the legacy file only as a fallback when the
    // Sailfish Secrets service is unavailable.
    return writeLegacyFile(server, user, secretStored ? QString() : password);
}

bool SecretStore::load(QString *server, QString *user, QString *password)
{
    const QJsonObject legacy = readLegacyObject();
    const QString legacyServer = legacy.value(QStringLiteral("server")).toString();
    const QString legacyUser = legacy.value(QStringLiteral("user")).toString();
    const QString legacyPassword = legacy.value(QStringLiteral("password")).toString();

    if (g_protectedMode) {
        g_usingSecrets = true;
        if (server) *server = legacyServer;
        if (user) *user = legacyUser;
        if (password) password->clear();
        return !legacyServer.isEmpty();
    }

    QString secretPassword;
    const bool secretRead = ensureCollection() && readNamedSecret(kSecretName, &secretPassword);

    if (secretRead && !secretPassword.isEmpty()) {
        g_usingSecrets = true;
        if (!legacyPassword.isEmpty()) {
            // Purge the plaintext fallback left by an older version.
            writeLegacyFile(legacyServer, legacyUser, QString());
        }
        if (server) *server = legacyServer;
        if (user) *user = legacyUser;
        if (password) *password = secretPassword;
        return !legacyServer.isEmpty();
    }

    if (!legacyPassword.isEmpty()) {
        g_usingSecrets = false;
        if (ensureCollection() && storeNamedSecret(kSecretName, legacyPassword)) {
            g_usingSecrets = true;
            writeLegacyFile(legacyServer, legacyUser, QString());
        }
        if (server) *server = legacyServer;
        if (user) *user = legacyUser;
        if (password) *password = legacyPassword;
        return !legacyServer.isEmpty();
    }

    // Recovery: the app password may live in the protected standalone secret
    // (e.g. the settings flag was lost). Reading it triggers the system
    // authentication prompt; if no protected secret exists this fails without
    // interaction.
    if (!legacyServer.isEmpty()) {
        QString protectedPassword;
        if (readStandalone(kProtectedSecretName, &protectedPassword)) {
            g_protectedMode = true;
            g_usingSecrets = true;
            if (server) *server = legacyServer;
            if (user) *user = legacyUser;
            if (password) *password = protectedPassword;
            return true;
        }
    }

    g_usingSecrets = false;
    if (server) *server = legacyServer;
    if (user) *user = legacyUser;
    if (password) password->clear();
    return false;
}

bool SecretStore::hasAccount()
{
    return !readLegacyObject().value(QStringLiteral("server")).toString().isEmpty();
}

void SecretStore::clear()
{
    if (ensureCollection()) {
        deleteNamedSecret(kSecretName);
    }
    deleteStandalone(kProtectedSecretName);
    QFile::remove(filePath());
    g_usingSecrets = false;
    g_protectedMode = false;
}

bool SecretStore::testSecrets(const QString &value, QString *out, QString *error)
{
    if (!ensureCollection()) {
        if (error) *error = g_lastError.isEmpty()
                ? QStringLiteral("collection_unavailable")
                : g_lastError;
        return false;
    }
    if (!storeNamedSecret(kTestSecretName, value)) {
        if (error) *error = g_lastError.isEmpty()
                ? QStringLiteral("store_failed")
                : g_lastError;
        return false;
    }

    QString readValue;
    if (!readNamedSecret(kTestSecretName, &readValue) || readValue != value) {
        if (error) *error = g_lastError.isEmpty()
                ? QStringLiteral("read_mismatch")
                : g_lastError;
        deleteNamedSecret(kTestSecretName);
        return false;
    }

    deleteNamedSecret(kTestSecretName);
    if (out) *out = readValue;
    return true;
}

void SecretStore::setProtectedMode(bool enabled)
{
    g_protectedMode = enabled;
}

bool SecretStore::protectedMode()
{
    return g_protectedMode;
}

bool SecretStore::storeProtected(const QString &password, QString *error)
{
    if (!ensureCollection()) {
        if (error) *error = g_lastError.isEmpty()
                ? QStringLiteral("collection_unavailable")
                : g_lastError;
        return false;
    }
    if (!storeStandalone(kProtectedSecretName, password)) {
        if (error) *error = g_lastError.isEmpty()
                ? QStringLiteral("protected_store_failed")
                : g_lastError;
        return false;
    }
    g_usingSecrets = true;
    return true;
}

bool SecretStore::readProtected(QString *password, QString *error)
{
    if (!readStandalone(kProtectedSecretName, password)) {
        if (error) *error = g_lastError.isEmpty()
                ? QStringLiteral("protected_read_failed")
                : g_lastError;
        return false;
    }
    g_usingSecrets = true;
    return true;
}

bool SecretStore::deleteProtected(QString *error)
{
    if (!deleteStandalone(kProtectedSecretName)) {
        if (error) *error = g_lastError.isEmpty()
                ? QStringLiteral("protected_delete_failed")
                : g_lastError;
        return false;
    }
    return true;
}

bool SecretStore::storeAppPassword(const QString &password)
{
    const bool ok = ensureCollection() && storeNamedSecret(kSecretName, password);
    g_usingSecrets = ok;
    return ok;
}

bool SecretStore::deleteAppPassword()
{
    return ensureCollection() && deleteNamedSecret(kSecretName);
}

bool SecretStore::testProtected(const QString &value, QString *out, QString *error)
{
    if (!ensureCollection()) {
        if (error) *error = g_lastError.isEmpty()
                ? QStringLiteral("collection_unavailable")
                : g_lastError;
        return false;
    }

    if (!storeStandalone(kProtectedTestName, value)) {
        if (error) *error = g_lastError.isEmpty()
                ? QStringLiteral("protected_store_failed")
                : g_lastError;
        return false;
    }

    QString readValue;
    if (!readStandalone(kProtectedTestName, &readValue) || readValue != value) {
        if (error) *error = g_lastError.isEmpty()
                ? QStringLiteral("protected_read_mismatch")
                : g_lastError;
        deleteStandalone(kProtectedTestName);
        return false;
    }

    deleteStandalone(kProtectedTestName);
    if (out) *out = readValue;
    return true;
}
