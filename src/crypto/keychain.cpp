#include "keychain.h"
#include "sodiumutils.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

Keychain::Keychain()
{
}

Keychain Keychain::fromEncryptedJson(const QString &hexBlob,
                                     const QString &masterPassword,
                                     bool *ok)
{
    Keychain result;

    bool hexOk = false;
    const QByteArray blob = Sodium::fromHex(hexBlob, &hexOk);
    if (!hexOk || blob.size() < 16 + 24 + 16) {
        if (ok) {
            *ok = false;
        }
        return result;
    }

    const QByteArray salt = blob.left(16);
    const QByteArray payload = blob.mid(16);

    const QByteArray key = Sodium::pwhash(masterPassword.toUtf8(), salt, 32);
    if (key.isEmpty()) {
        if (ok) {
            *ok = false;
        }
        return result;
    }

    const QByteArray plaintext = Sodium::decrypt(payload, key);
    if (plaintext.isEmpty()) {
        if (ok) {
            *ok = false;
        }
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(plaintext, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (ok) {
            *ok = false;
        }
        return result;
    }

    const QJsonObject root = document.object();
    const QJsonObject keys = root.value(QStringLiteral("keys")).toObject();
    for (QJsonObject::const_iterator it = keys.constBegin(); it != keys.constEnd(); ++it) {
        const QByteArray keyBytes = Sodium::fromHex(it.value().toString());
        if (keyBytes.size() == 32) {
            result.m_keys.insert(it.key(), keyBytes);
        }
    }
    result.m_current = root.value(QStringLiteral("current")).toString();

    if (result.m_keys.isEmpty() || !result.m_keys.contains(result.m_current)) {
        if (ok) {
            *ok = false;
        }
        return Keychain();
    }

    if (ok) {
        *ok = true;
    }
    return result;
}

bool Keychain::isEmpty() const
{
    return m_keys.isEmpty() || m_current.isEmpty();
}

QByteArray Keychain::key(const QString &id) const
{
    return m_keys.value(id);
}

QStringList Keychain::ids() const
{
    return m_keys.keys();
}
