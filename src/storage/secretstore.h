#ifndef SECRETSTORE_H
#define SECRETSTORE_H

#include <QString>

class SecretStore
{
public:
    static bool save(const QString &server, const QString &user, const QString &password);
    static bool load(QString *server, QString *user, QString *password);
    static bool hasAccount();
    static void clear();

    static bool usingSecrets();
    static QString lastError();
    static bool testSecrets(const QString &value, QString *out, QString *error = 0);
    static bool testProtected(const QString &value, QString *out, QString *error = 0);

    static void setProtectedMode(bool enabled);
    static bool protectedMode();
    static bool storeProtected(const QString &password, QString *error = 0);
    static bool readProtected(QString *password, QString *error = 0);
    static bool deleteProtected(QString *error = 0);
    static bool storeAppPassword(const QString &password);
    static bool deleteAppPassword();

    static QString directoryPath();
    static QString filePath();

private:
    static bool ensureCollection();
    static bool storeSecret(const QString &password);
    static bool readSecret(QString *password);
    static bool deleteSecret();
    static bool writeLegacyFile(const QString &server,
                                const QString &user,
                                const QString &password);
};

#endif // SECRETSTORE_H
