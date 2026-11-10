#include "challenge.h"
#include "sodiumutils.h"

#include <QJsonArray>
#include <QJsonValue>

PwdChallenge::PwdChallenge()
{
}

PwdChallenge PwdChallenge::fromJson(const QJsonObject &challenge, bool *ok)
{
    PwdChallenge result;

    const QJsonValue typeValue = challenge.value(QStringLiteral("type"));
    if (typeValue.isString()) {
        result.m_type = typeValue.toString();
    } else {
        result.m_type = QStringLiteral("PWDv1r1");
    }

    const QJsonValue saltsValue = challenge.value(QStringLiteral("salts"));
    if (!saltsValue.isArray()) {
        if (ok) {
            *ok = false;
        }
        return result;
    }

    const QJsonArray salts = saltsValue.toArray();
    if (salts.size() != 3) {
        if (ok) {
            *ok = false;
        }
        return result;
    }

    bool saltOk = false;
    result.m_passwordSalt = Sodium::fromHex(salts.at(0).toString(), &saltOk);
    if (!saltOk) {
        if (ok) {
            *ok = false;
        }
        return result;
    }

    result.m_genericHashKey = Sodium::fromHex(salts.at(1).toString(), &saltOk);
    if (!saltOk) {
        if (ok) {
            *ok = false;
        }
        return result;
    }

    result.m_passwordHashSalt = Sodium::fromHex(salts.at(2).toString(), &saltOk);
    if (!saltOk) {
        if (ok) {
            *ok = false;
        }
        return result;
    }

    if (ok) {
        *ok = result.isValid();
    }
    return result;
}

bool PwdChallenge::isValid() const
{
    return !m_passwordSalt.isEmpty()
            && !m_genericHashKey.isEmpty()
            && m_passwordHashSalt.size() == 16;
}

QString PwdChallenge::solve(const QString &masterPassword) const
{
    if (!isValid()) {
        return QString();
    }

    const QByteArray password = masterPassword.toUtf8();
    if (password.size() < 12 || password.size() > 128) {
        return QString();
    }

    // BLAKE2b-512 over password || passwordSalt, keyed with genericHashKey.
    const QByteArray genericHash = Sodium::generichash(password + m_passwordSalt,
                                                       m_genericHashKey,
                                                       64);
    if (genericHash.isEmpty()) {
        return QString();
    }

    // Argon2id13 with the interactive parameters used by the official clients.
    const QByteArray passwordHash = Sodium::pwhash(genericHash, m_passwordHashSalt, 32);
    if (passwordHash.isEmpty()) {
        return QString();
    }

    return Sodium::toHex(passwordHash);
}
