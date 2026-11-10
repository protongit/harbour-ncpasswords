#include "sodiumutils.h"

#include <sodium.h>

namespace {

bool g_initialized = false;
bool g_available = false;

bool ensureInitialized()
{
    if (!g_initialized) {
        g_initialized = true;
        g_available = sodium_init() >= 0;
    }
    return g_available;
}

} // namespace

namespace Sodium {

bool init()
{
    return ensureInitialized();
}

QByteArray randomBytes(int size)
{
    if (!ensureInitialized() || size <= 0) {
        return QByteArray();
    }
    QByteArray out(size, 0);
    randombytes_buf(out.data(), size);
    return out;
}

QByteArray generichash(const QByteArray &message, const QByteArray &key, int outSize)
{
    if (!ensureInitialized() || outSize < static_cast<int>(crypto_generichash_BYTES_MIN)
            || outSize > static_cast<int>(crypto_generichash_BYTES_MAX)) {
        return QByteArray();
    }
    if (!key.isEmpty() && (key.size() < static_cast<int>(crypto_generichash_KEYBYTES_MIN)
                           || key.size() > static_cast<int>(crypto_generichash_KEYBYTES_MAX))) {
        return QByteArray();
    }

    QByteArray out(outSize, 0);
    if (crypto_generichash(reinterpret_cast<unsigned char *>(out.data()),
                           static_cast<size_t>(out.size()),
                           reinterpret_cast<const unsigned char *>(message.constData()),
                           static_cast<unsigned long long>(message.size()),
                           key.isEmpty() ? 0
                                         : reinterpret_cast<const unsigned char *>(key.constData()),
                           static_cast<size_t>(key.size())) != 0) {
        return QByteArray();
    }
    return out;
}

QByteArray pwhash(const QByteArray &password, const QByteArray &salt, int outSize)
{
    if (!ensureInitialized() || outSize <= 0
            || salt.size() != crypto_pwhash_SALTBYTES) {
        return QByteArray();
    }

    QByteArray out(outSize, 0);
    if (crypto_pwhash(reinterpret_cast<unsigned char *>(out.data()),
                      static_cast<unsigned long long>(out.size()),
                      password.constData(),
                      static_cast<unsigned long long>(password.size()),
                      reinterpret_cast<const unsigned char *>(salt.constData()),
                      crypto_pwhash_OPSLIMIT_INTERACTIVE,
                      crypto_pwhash_MEMLIMIT_INTERACTIVE,
                      crypto_pwhash_ALG_ARGON2ID13) != 0) {
        return QByteArray();
    }
    return out;
}

QByteArray secretbox(const QByteArray &message, const QByteArray &nonce, const QByteArray &key)
{
    if (!ensureInitialized()
            || nonce.size() != crypto_secretbox_NONCEBYTES
            || key.size() != crypto_secretbox_KEYBYTES) {
        return QByteArray();
    }

    QByteArray out(message.size() + crypto_secretbox_MACBYTES, 0);
    if (crypto_secretbox_easy(reinterpret_cast<unsigned char *>(out.data()),
                              reinterpret_cast<const unsigned char *>(message.constData()),
                              static_cast<unsigned long long>(message.size()),
                              reinterpret_cast<const unsigned char *>(nonce.constData()),
                              reinterpret_cast<const unsigned char *>(key.constData())) != 0) {
        return QByteArray();
    }
    return out;
}

QByteArray secretboxOpen(const QByteArray &ciphertext, const QByteArray &nonce, const QByteArray &key)
{
    if (!ensureInitialized()
            || nonce.size() != crypto_secretbox_NONCEBYTES
            || key.size() != crypto_secretbox_KEYBYTES
            || ciphertext.size() < static_cast<int>(crypto_secretbox_MACBYTES)) {
        return QByteArray();
    }

    QByteArray out(ciphertext.size() - crypto_secretbox_MACBYTES, 0);
    if (crypto_secretbox_open_easy(reinterpret_cast<unsigned char *>(out.data()),
                                   reinterpret_cast<const unsigned char *>(ciphertext.constData()),
                                   static_cast<unsigned long long>(ciphertext.size()),
                                   reinterpret_cast<const unsigned char *>(nonce.constData()),
                                   reinterpret_cast<const unsigned char *>(key.constData())) != 0) {
        return QByteArray();
    }
    return out;
}

QByteArray encrypt(const QByteArray &message, const QByteArray &key)
{
    const QByteArray nonce = randomBytes(crypto_secretbox_NONCEBYTES);
    const QByteArray box = secretbox(message, nonce, key);
    if (nonce.isEmpty() || box.isEmpty()) {
        return QByteArray();
    }
    return nonce + box;
}

QByteArray decrypt(const QByteArray &blob, const QByteArray &key)
{
    if (blob.size() < static_cast<int>(crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES)) {
        return QByteArray();
    }
    const QByteArray nonce = blob.left(crypto_secretbox_NONCEBYTES);
    const QByteArray ciphertext = blob.mid(crypto_secretbox_NONCEBYTES);
    return secretboxOpen(ciphertext, nonce, key);
}

QByteArray fromHex(const QString &hex, bool *ok)
{
    const QByteArray latin = hex.toLatin1();
    if (latin.size() % 2 != 0) {
        if (ok) {
            *ok = false;
        }
        return QByteArray();
    }

    QByteArray out(latin.size() / 2, 0);
    size_t written = 0;
    if (sodium_hex2bin(reinterpret_cast<unsigned char *>(out.data()),
                       static_cast<size_t>(out.size()),
                       latin.constData(),
                       static_cast<size_t>(latin.size()),
                       0,
                       &written,
                       0) != 0) {
        if (ok) {
            *ok = false;
        }
        return QByteArray();
    }
    out.resize(static_cast<int>(written));
    if (ok) {
        *ok = true;
    }
    return out;
}

QString toHex(const QByteArray &data)
{
    if (data.isEmpty()) {
        return QString();
    }
    QByteArray out(data.size() * 2 + 1, 0);
    sodium_bin2hex(out.data(), static_cast<size_t>(out.size()),
                   reinterpret_cast<const unsigned char *>(data.constData()),
                   static_cast<size_t>(data.size()));
    out.chop(1);
    return QString::fromLatin1(out).toLower();
}

bool constantEquals(const QByteArray &a, const QByteArray &b)
{
    return a.size() == b.size()
            && sodium_memcmp(a.constData(), b.constData(),
                             static_cast<size_t>(a.size())) == 0;
}

void wipe(QByteArray &data)
{
    if (!data.isEmpty()) {
        sodium_memzero(data.data(), static_cast<size_t>(data.size()));
        data.fill(0);
    }
    data.clear();
}

} // namespace Sodium
