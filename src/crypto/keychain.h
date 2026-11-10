#ifndef KEYCHAIN_H
#define KEYCHAIN_H

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QStringList>

class Keychain
{
public:
    Keychain();

    static Keychain fromEncryptedJson(const QString &hexBlob,
                                      const QString &masterPassword,
                                      bool *ok = 0);

    bool isEmpty() const;
    QString currentId() const { return m_current; }
    QByteArray currentKey() const { return key(m_current); }
    QByteArray key(const QString &id) const;
    QStringList ids() const;

private:
    QMap<QString, QByteArray> m_keys;
    QString m_current;
};

#endif // KEYCHAIN_H
