#ifndef TOTP_H
#define TOTP_H

#include <QString>

struct TotpConfig
{
    QString secret;       // Base32, as stored
    QString type;         // "totp" or "hotp"
    QString algorithm;    // "sha1", "sha256" or "sha512"
    QString issuer;
    QString accountName;
    int digits = 6;
    int period = 30;
    qint64 counter = 0;
    bool valid = false;
};

namespace Totp {

// Parses either the JSON value used by the Nextcloud Passwords clients
// (custom field "client.ios.otp") or an otpauth:// URI.
TotpConfig fromValue(const QString &value);

QString code(const TotpConfig &config, qint64 unixSeconds);
int remaining(const TotpConfig &config, qint64 unixSeconds);
QString formattedCode(const QString &code, int digits);

} // namespace Totp

#endif // TOTP_H
