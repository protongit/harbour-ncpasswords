#ifndef CRYPTOOPS_H
#define CRYPTOOPS_H

#include "keychain.h"
#include "models.h"

#include <QJsonObject>
#include <QString>

bool decryptPassword(PasswordEntry &entry, const Keychain &keychain, QString *error = 0);
bool decryptFolder(Folder &folder, const Keychain &keychain, QString *error = 0);
bool decryptTag(Tag &tag, const Keychain &keychain, QString *error = 0);

QJsonObject passwordPayload(const PasswordEntry &entry, const Keychain &keychain);
QJsonObject folderPayload(const Folder &folder, const Keychain &keychain);
QJsonObject tagPayload(const Tag &tag, const Keychain &keychain);

QString sha1Hex(const QString &text);

#endif // CRYPTOOPS_H
