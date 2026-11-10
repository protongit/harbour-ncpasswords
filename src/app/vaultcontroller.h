#ifndef VAULTCONTROLLER_H
#define VAULTCONTROLLER_H

#include "challenge.h"
#include "keychain.h"
#include "models.h"
#include "passwordlistmodel.h"
#include "totp.h"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <functional>

class QTimer;

class VaultController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString errorCode READ errorCode NOTIFY errorCodeChanged)
    Q_PROPERTY(QString errorDetail READ errorDetail NOTIFY errorCodeChanged)
    Q_PROPERTY(bool hasAccount READ hasAccount NOTIFY accountChanged)
    Q_PROPERTY(QString server READ server NOTIFY accountChanged)
    Q_PROPERTY(QString user READ user NOTIFY accountChanged)
    Q_PROPERTY(bool cseEnabled READ cseEnabled NOTIFY cseEnabledChanged)
    Q_PROPERTY(PasswordListModel *passwords READ passwords CONSTANT)
    Q_PROPERTY(PasswordListModel *trash READ trash CONSTANT)
    Q_PROPERTY(PasswordListModel *folderPasswords READ folderPasswords CONSTANT)
    Q_PROPERTY(QVariantList folders READ folders NOTIFY foldersChanged)
    Q_PROPERTY(QVariantList tags READ tags NOTIFY tagsChanged)
    Q_PROPERTY(int lockSeconds READ lockSeconds WRITE setLockSeconds NOTIFY lockSecondsChanged)
    Q_PROPERTY(int clipboardSeconds READ clipboardSeconds WRITE setClipboardSeconds NOTIFY clipboardSecondsChanged)
    Q_PROPERTY(QString passwordStrength READ passwordStrength WRITE setPasswordStrength NOTIFY passwordStrengthChanged)
    Q_PROPERTY(QString searchText READ searchText NOTIFY filtersChanged)
    Q_PROPERTY(QString folderFilter READ folderFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString tagFilter READ tagFilter NOTIFY filtersChanged)
    Q_PROPERTY(bool loginFlowActive READ loginFlowActive NOTIFY loginFlowActiveChanged)
    Q_PROPERTY(bool secretsActive READ secretsActive NOTIFY secretsActiveChanged)
    Q_PROPERTY(int totpCount READ totpCount NOTIFY totpChanged)
    Q_PROPERTY(int totpIndex READ totpIndex WRITE setTotpIndex NOTIFY totpChanged)
    Q_PROPERTY(QVariantMap currentTotp READ currentTotp NOTIFY totpChanged)
    Q_PROPERTY(bool fingerprintLock READ fingerprintLock NOTIFY fingerprintLockChanged)
    Q_PROPERTY(bool fingerprintRequired READ fingerprintRequired NOTIFY fingerprintRequiredChanged)
    Q_PROPERTY(bool pinEnabled READ pinEnabled NOTIFY pinEnabledChanged)
    Q_PROPERTY(bool pinRequired READ pinRequired NOTIFY pinRequiredChanged)
    Q_PROPERTY(QString viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

public:
    explicit VaultController(QObject *parent = 0);
    ~VaultController();

    QString state() const { return m_state; }
    bool busy() const { return m_busy; }
    QString errorCode() const { return m_errorCode; }
    QString errorDetail() const { return m_errorDetail; }
    bool hasAccount() const { return m_hasAccount; }
    QString server() const { return m_server; }
    QString user() const { return m_user; }
    bool cseEnabled() const { return m_cse; }
    PasswordListModel *passwords() const { return m_passwords; }
    PasswordListModel *trash() const { return m_trash; }
    PasswordListModel *folderPasswords() const { return m_folderPasswords; }
    QVariantList folders() const;
    QVariantList tags() const;
    int lockSeconds() const { return m_lockSeconds; }
    int clipboardSeconds() const { return m_clipboardSeconds; }
    QString passwordStrength() const { return m_passwordStrength; }
    void setPasswordStrength(const QString &strength);
    QString searchText() const { return m_search; }
    QString folderFilter() const { return m_folderFilter; }
    QString tagFilter() const { return m_tagFilter; }
    bool loginFlowActive() const { return m_loginFlowActive; }
    bool secretsActive() const { return m_secretsActive; }
    int totpCount() const { return m_totpServices.size(); }
    int totpIndex() const { return m_totpIndex; }
    QVariantMap currentTotp() const;
    bool fingerprintLock() const { return m_fingerprintLock; }
    bool fingerprintRequired() const { return m_fingerprintRequired; }
    bool pinEnabled() const { return m_pinEnabled; }
    bool pinRequired() const { return m_pinRequired; }
    QString viewMode() const { return m_viewMode; }
    void setViewMode(const QString &mode);
    QString language() const { return m_language; }
    void setLanguage(const QString &language);

    void setLockSeconds(int seconds);
    void setClipboardSeconds(int seconds);

    Q_INVOKABLE void restoreAccount();
    Q_INVOKABLE void startLoginFlow(const QString &server);
    Q_INVOKABLE void cancelLoginFlow();
    Q_INVOKABLE void connectToServer(const QString &server,
                                     const QString &user,
                                     const QString &appPassword);
    Q_INVOKABLE void unlock(const QString &masterPassword);
    Q_INVOKABLE void reconnect();
    Q_INVOKABLE void authenticateWithFingerprint();
    Q_INVOKABLE void completeFingerprintAuth();
    Q_INVOKABLE void fingerprintAuthFailed(const QString &detail);
    Q_INVOKABLE void verifyPin(const QString &pin);
    Q_INVOKABLE void enablePin(const QString &pin);
    Q_INVOKABLE void changePin(const QString &currentPin, const QString &newPin);
    Q_INVOKABLE void disablePin(const QString &currentPin);
    Q_INVOKABLE void setFingerprintLock(bool enabled);
    Q_INVOKABLE void lock();
    Q_INVOKABLE void wipeAccount();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void openFolder(const QString &folderId);
    Q_INVOKABLE void closeFolder();
    Q_INVOKABLE void setFolderSearch(const QString &text);
    Q_INVOKABLE void reloadTrash();

    Q_INVOKABLE void setSearch(const QString &text);
    Q_INVOKABLE void setFolderFilter(const QString &folderId);
    Q_INVOKABLE void setTagFilter(const QString &tagId);
    Q_INVOKABLE void clearFilters();

    Q_INVOKABLE QVariantMap password(const QString &id) const;
    Q_INVOKABLE void createPassword(const QVariantMap &data);
    Q_INVOKABLE void updatePassword(const QVariantMap &data);
    Q_INVOKABLE void deletePassword(const QString &id);
    Q_INVOKABLE void restorePassword(const QString &id);

    Q_INVOKABLE void createFolder(const QString &label, const QString &parentId);
    Q_INVOKABLE void updateFolder(const QString &id, const QString &label, const QString &parentId);
    Q_INVOKABLE void deleteFolder(const QString &id);

    Q_INVOKABLE void createTag(const QString &label, const QString &color);
    Q_INVOKABLE void updateTag(const QString &id, const QString &label, const QString &color);
    Q_INVOKABLE void deleteTag(const QString &id);

    Q_INVOKABLE void copySecret(const QString &text);
    Q_INVOKABLE QString generatePassword(int length, bool digits, bool symbols);
    Q_INVOKABLE QString generatePasswordForStrength();
    Q_INVOKABLE void noteActivity();
    Q_INVOKABLE QString folderLabel(const QString &id) const;
    Q_INVOKABLE QString tagLabel(const QString &id) const;
    Q_INVOKABLE void nextTotp();
    Q_INVOKABLE void previousTotp();
    Q_INVOKABLE void setTotpIndex(int index);
    Q_INVOKABLE QVariantMap totpForPassword(const QString &id) const;
    Q_INVOKABLE bool hasTotp(const QString &id) const;
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void logDebug(const QString &text);

    void handleApplicationState(int state);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void stateChanged();
    void busyChanged();
    void errorCodeChanged();
    void accountChanged();
    void cseEnabledChanged();
    void foldersChanged();
    void tagsChanged();
    void lockSecondsChanged();
    void clipboardSecondsChanged();
    void passwordStrengthChanged();
    void filtersChanged();
    void loginFlowActiveChanged();
    void secretsActiveChanged();
    void totpChanged();
    void fingerprintLockChanged();
    void fingerprintRequiredChanged();
    void pinEnabledChanged();
    void pinRequiredChanged();
    void viewModeChanged();
    void languageChanged();
    void loginFlowUrl(const QString &url);
    void toast(const QString &key);

private:
    struct SessionResult
    {
        bool ok = false;
        bool cse = false;
        QString code;
        QString detail;
        QString sessionId;
        PwdChallenge challenge;
    };

    struct UnlockResult
    {
        bool ok = false;
        QString code;
        QString detail;
        QString sessionId;
        Keychain keychain;
    };

    struct LoadResult
    {
        bool ok = false;
        QString code;
        QString detail;
        QVector<PasswordEntry> passwords;
        QVector<PasswordEntry> trash;
        QVector<Folder> folders;
        QVector<Tag> tags;
        QString debug;
    };

    struct TotpService
    {
        QString passwordId;
        QString entryLabel;
        TotpConfig config;
    };

    struct OpResult
    {
        bool ok = false;
        int status = 0;
        QString code;
        QString detail;
    };

    template <typename T>
    void runTask(std::function<T()> work, std::function<void(const T &)> done)
    {
        // Network operations must run in the main thread on Sailfish OS: a
        // QNetworkAccessManager created in a worker thread never receives
        // replies here. The calls are fast enough to run synchronously with a
        // busy indicator; only the Argon2 derivation is noticeably slow.
        const T result = work();
        done(result);
    }

    void setState(const QString &state);
    void setBusy(bool busy);
    void setError(const QString &code, const QString &detail = QString());
    void startSessionRequest();
    void startLoad();
    void applyLoaded(const LoadResult &result);
    void rebuildFolderLabels();
    void rebuildTotpServices();
    bool pinMatches(const QString &pin) const;
    void storePin(const QString &pin);
    void startKeepAlive();
    void stopKeepAlive();
    void keepAlive();
    void pollLoginFlow();
    void runOperation(const QString &verb,
                      const QString &path,
                      const QJsonObject &payload,
                      const QString &successKey);

    PasswordEntry entryFromMap(const QVariantMap &data) const;
    Folder folderById(const QString &id) const;
    Tag tagById(const QString &id) const;
    static QList<CustomField> customFieldsFromVariant(const QVariant &value);
    static QVariantList customFieldsToVariant(const QList<CustomField> &fields);

    QString m_state;
    bool m_busy = false;
    QString m_errorCode;
    QString m_errorDetail;
    bool m_hasAccount = false;
    bool m_restored = false;
    QString m_server;
    QString m_user;
    QString m_appPassword;
    bool m_cse = false;

    QString m_sessionId;
    Keychain m_keychain;
    PwdChallenge m_challenge;

    PasswordListModel *m_passwords;
    PasswordListModel *m_trash;
    PasswordListModel *m_folderPasswords;
    QString m_openFolderId;

    QVector<Folder> m_folders;
    QVector<Tag> m_tags;
    QHash<QString, QString> m_folderLabels;
    QHash<QString, QString> m_tagLabels;

    QString m_search;
    QString m_folderFilter;
    QString m_tagFilter;

    QTimer *m_lockTimer;
    QTimer *m_keepAliveTimer;
    int m_lockSeconds;
    int m_clipboardSeconds;
    QString m_passwordStrength;

    bool m_loginFlowActive = false;
    bool m_secretsActive = false;
    bool m_fingerprintLock = false;
    bool m_fingerprintRequired = false;
    bool m_pinEnabled = false;
    bool m_pinRequired = false;
    QByteArray m_pinSalt;
    QByteArray m_pinHash;
    int m_pinFailures = 0;
    qint64 m_pinLockedUntil = 0;
    QString m_viewMode;
    QString m_language;
    QVector<TotpService> m_totpServices;
    int m_totpIndex = 0;
    QTimer *m_totpTimer;
    QTimer *m_loginPollTimer;
    QString m_loginServer;
    QString m_loginEndpoint;
    QString m_loginToken;
};

#endif // VAULTCONTROLLER_H
