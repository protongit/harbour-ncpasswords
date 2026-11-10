#ifndef SODIUMUTILS_H
#define SODIUMUTILS_H

#include <QByteArray>
#include <QString>

namespace Sodium {

bool init();

QByteArray randomBytes(int size);

QByteArray generichash(const QByteArray &message, const QByteArray &key, int outSize = 64);
QByteArray pwhash(const QByteArray &password, const QByteArray &salt, int outSize = 32);

QByteArray secretbox(const QByteArray &message, const QByteArray &nonce, const QByteArray &key);
QByteArray secretboxOpen(const QByteArray &ciphertext, const QByteArray &nonce, const QByteArray &key);

// Returns nonce || secretbox ciphertext.
QByteArray encrypt(const QByteArray &message, const QByteArray &key);
// Input is nonce || secretbox ciphertext.
QByteArray decrypt(const QByteArray &blob, const QByteArray &key);

QByteArray fromHex(const QString &hex, bool *ok = 0);
QString toHex(const QByteArray &data);

void wipe(QByteArray &data);
bool constantEquals(const QByteArray &a, const QByteArray &b);

} // namespace Sodium

#endif // SODIUMUTILS_H
