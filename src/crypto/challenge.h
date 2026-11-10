#ifndef CHALLENGE_H
#define CHALLENGE_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>

class PwdChallenge
{
public:
    PwdChallenge();

    static PwdChallenge fromJson(const QJsonObject &challenge, bool *ok = 0);

    bool isValid() const;
    QString type() const { return m_type; }

    // Returns the lower-case hex challenge solution, or an empty string on failure.
    QString solve(const QString &masterPassword) const;

private:
    QString m_type;
    QByteArray m_passwordSalt;
    QByteArray m_genericHashKey;
    QByteArray m_passwordHashSalt;
};

#endif // CHALLENGE_H
