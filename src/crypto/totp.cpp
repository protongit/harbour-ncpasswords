#include "totp.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QUrlQuery>

namespace {

QByteArray base32Decode(const QString &input)
{
    QByteArray output;
    quint32 buffer = 0;
    int bits = 0;

    for (const QChar &raw : input) {
        if (raw.isSpace() || raw == QLatin1Char('-')) {
            continue;
        }
        if (raw == QLatin1Char('=')) {
            break;
        }

        const QChar c = raw.toUpper();
        int value = -1;
        if (c >= QLatin1Char('A') && c <= QLatin1Char('Z')) {
            value = c.unicode() - QLatin1Char('A').unicode();
        } else if (c >= QLatin1Char('2') && c <= QLatin1Char('7')) {
            value = 26 + c.unicode() - QLatin1Char('2').unicode();
        }
        if (value < 0) {
            return QByteArray();
        }

        buffer = (buffer << 5) | static_cast<quint32>(value);
        bits += 5;
        if (bits >= 8) {
            output.append(static_cast<char>((buffer >> (bits - 8)) & 0xff));
            bits -= 8;
            buffer &= (1u << bits) - 1u;
        }
    }

    return output;
}

QByteArray hmac(const QByteArray &key, const QByteArray &message, const QString &algorithm)
{
    QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Sha1;
    int blockSize = 64;

    if (algorithm == QLatin1String("sha256")) {
        hashAlgorithm = QCryptographicHash::Sha256;
    } else if (algorithm == QLatin1String("sha512")) {
        hashAlgorithm = QCryptographicHash::Sha512;
        blockSize = 128;
    }

    QByteArray shortKey = key;
    if (shortKey.size() > blockSize) {
        shortKey = QCryptographicHash::hash(shortKey, hashAlgorithm);
    }
    shortKey = shortKey.leftJustified(blockSize, '\0', true);

    QByteArray innerPad(blockSize, 0x36);
    QByteArray outerPad(blockSize, 0x5c);
    for (int i = 0; i < blockSize; ++i) {
        innerPad[i] = static_cast<char>(innerPad.at(i) ^ shortKey.at(i));
        outerPad[i] = static_cast<char>(outerPad.at(i) ^ shortKey.at(i));
    }

    const QByteArray inner = QCryptographicHash::hash(innerPad + message, hashAlgorithm);
    return QCryptographicHash::hash(outerPad + inner, hashAlgorithm);
}

QString normalizedAlgorithm(const QString &value)
{
    const QString algorithm = value.trimmed().toLower();
    if (algorithm == QLatin1String("sha256") || algorithm == QLatin1String("sha512")) {
        return algorithm;
    }
    return QStringLiteral("sha1");
}

QString normalizedType(const QString &value)
{
    return value.trimmed().toLower() == QLatin1String("hotp")
            ? QStringLiteral("hotp")
            : QStringLiteral("totp");
}

TotpConfig fromJson(const QString &value)
{
    TotpConfig config;
    const QJsonDocument document = QJsonDocument::fromJson(value.toUtf8());
    if (!document.isObject()) {
        return config;
    }
    const QJsonObject object = document.object();

    config.secret = object.value(QStringLiteral("secret")).toString();
    config.type = normalizedType(object.value(QStringLiteral("type")).toString());
    config.algorithm = normalizedAlgorithm(object.value(QStringLiteral("algorithm")).toString());
    config.digits = object.value(QStringLiteral("digits")).toInt(6);
    config.period = object.value(QStringLiteral("period")).toInt(30);
    config.counter = static_cast<qint64>(object.value(QStringLiteral("counter")).toDouble());
    config.issuer = object.value(QStringLiteral("issuer")).toString();
    config.accountName = object.value(QStringLiteral("accountName")).toString();

    if (config.digits < 6 || config.digits > 9) {
        config.digits = 6;
    }
    if (config.period <= 0) {
        config.period = 30;
    }
    config.valid = !config.secret.trimmed().isEmpty()
            && !base32Decode(config.secret).isEmpty();
    return config;
}

TotpConfig fromUrl(const QString &value)
{
    TotpConfig config;
    const QUrl url(value);
    if (url.scheme().compare(QLatin1String("otpauth"), Qt::CaseInsensitive) != 0) {
        return config;
    }

    config.type = normalizedType(url.host());
    config.algorithm = QStringLiteral("sha1");
    config.digits = 6;
    config.period = 30;

    QString label = url.path();
    while (label.startsWith(QLatin1Char('/'))) {
        label.remove(0, 1);
    }
    if (!label.isEmpty()) {
        const QStringList parts = label.split(QLatin1Char(':'), QString::KeepEmptyParts);
        if (parts.size() == 2) {
            config.issuer = QUrl::fromPercentEncoding(parts.at(0).toUtf8()).trimmed();
            config.accountName = QUrl::fromPercentEncoding(parts.at(1).toUtf8()).trimmed();
        } else {
            config.accountName = QUrl::fromPercentEncoding(label.toUtf8()).trimmed();
        }
    }

    const QUrlQuery query(url);
    config.secret = query.queryItemValue(QStringLiteral("secret"));
    if (query.hasQueryItem(QStringLiteral("issuer"))) {
        config.issuer = query.queryItemValue(QStringLiteral("issuer"));
    }
    if (query.hasQueryItem(QStringLiteral("algorithm"))) {
        config.algorithm = normalizedAlgorithm(query.queryItemValue(QStringLiteral("algorithm")));
    }
    if (query.hasQueryItem(QStringLiteral("digits"))) {
        config.digits = query.queryItemValue(QStringLiteral("digits")).toInt();
    }
    if (query.hasQueryItem(QStringLiteral("period"))) {
        config.period = query.queryItemValue(QStringLiteral("period")).toInt();
    }
    if (query.hasQueryItem(QStringLiteral("counter"))) {
        config.counter = query.queryItemValue(QStringLiteral("counter")).toLongLong();
    }

    if (config.digits < 6 || config.digits > 9) {
        config.digits = 6;
    }
    if (config.period <= 0) {
        config.period = 30;
    }
    config.valid = !config.secret.trimmed().isEmpty()
            && !base32Decode(config.secret).isEmpty();
    return config;
}

} // namespace

namespace Totp {

TotpConfig fromValue(const QString &value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.startsWith(QLatin1String("otpauth://"), Qt::CaseInsensitive)) {
        return fromUrl(trimmed);
    }
    return fromJson(trimmed);
}

QString code(const TotpConfig &config, qint64 unixSeconds)
{
    if (!config.valid || config.digits < 6 || config.digits > 9 || config.period <= 0) {
        return QString();
    }

    const QByteArray key = base32Decode(config.secret);
    if (key.isEmpty()) {
        return QString();
    }

    const qint64 counter = (config.type == QLatin1String("hotp"))
            ? config.counter
            : (unixSeconds / config.period);

    QByteArray message(8, 0);
    quint64 value = static_cast<quint64>(counter);
    for (int i = 7; i >= 0; --i) {
        message[i] = static_cast<char>(value & 0xff);
        value >>= 8;
    }

    const QByteArray hash = hmac(key, message, config.algorithm);
    if (hash.size() < 20) {
        return QString();
    }

    const int offset = hash.at(hash.size() - 1) & 0x0f;
    if (offset + 4 > hash.size()) {
        return QString();
    }

    const quint32 binary = ((static_cast<quint32>(hash.at(offset) & 0x7f) << 24)
                            | (static_cast<quint32>(hash.at(offset + 1) & 0xff) << 16)
                            | (static_cast<quint32>(hash.at(offset + 2) & 0xff) << 8)
                            | (static_cast<quint32>(hash.at(offset + 3) & 0xff)));

    quint32 modulus = 1;
    for (int i = 0; i < config.digits; ++i) {
        modulus *= 10;
    }

    const quint32 otp = binary % modulus;
    return QString::number(otp).rightJustified(config.digits, QLatin1Char('0'));
}

int remaining(const TotpConfig &config, qint64 unixSeconds)
{
    if (!config.valid || config.period <= 0) {
        return 0;
    }
    if (config.type == QLatin1String("hotp")) {
        return 0;
    }
    const int elapsed = static_cast<int>(unixSeconds % config.period);
    return config.period - elapsed;
}

QString formattedCode(const QString &code, int digits)
{
    if (code.isEmpty()) {
        return QString();
    }
    if (digits == 6 && code.size() == 6) {
        return code.left(3) + QLatin1Char(' ') + code.mid(3);
    }
    if (digits == 8 && code.size() == 8) {
        return code.left(4) + QLatin1Char(' ') + code.mid(4);
    }
    return code;
}

} // namespace Totp
