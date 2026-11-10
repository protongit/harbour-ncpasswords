#include "cryptoops.h"
#include "sodiumutils.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>

namespace {

QString encryptField(const QString &value, const QByteArray &key)
{
    if (value.isEmpty() || key.isEmpty()) {
        return value;
    }
    const QByteArray box = Sodium::encrypt(value.toUtf8(), key);
    if (box.isEmpty()) {
        return QString();
    }
    return Sodium::toHex(box);
}

bool decryptField(const QString &value, const QByteArray &key, QString *result)
{
    if (value.isEmpty()) {
        *result = QString();
        return true;
    }
    if (key.isEmpty()) {
        return false;
    }

    bool hexOk = false;
    QByteArray blob = Sodium::fromHex(value, &hexOk);
    if (!hexOk || blob.isEmpty()) {
        blob = QByteArray::fromBase64(value.toLatin1());
    }
    const QByteArray plaintext = Sodium::decrypt(blob, key);
    if (plaintext.isEmpty()) {
        return false;
    }
    *result = QString::fromUtf8(plaintext);
    return true;
}

QString customFieldsJson(const PasswordEntry &entry)
{
    if (!entry.customFields.isEmpty()) {
        QJsonArray array;
        for (const CustomField &field : entry.customFields) {
            array.append(field.toJson());
        }
        return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
    }
    return entry.customFieldsJson.isEmpty() ? QStringLiteral("[]") : entry.customFieldsJson;
}

qint64 nowSeconds()
{
    return QDateTime::currentMSecsSinceEpoch() / 1000;
}

} // namespace

QString sha1Hex(const QString &text)
{
    return QString::fromLatin1(QCryptographicHash::hash(text.toUtf8(),
                                                        QCryptographicHash::Sha1).toHex());
}

bool decryptPassword(PasswordEntry &entry, const Keychain &keychain, QString *error)
{
    if (entry.cseType.isEmpty() || entry.cseType == QLatin1String("none")) {
        entry.customFields = customFieldsFromJson(entry.customFieldsJson);
        return true;
    }
    if (entry.cseType != QLatin1String("CSEv1r1")) {
        if (error) {
            *error = QStringLiteral("unsupported_encryption");
        }
        return false;
    }
    if (keychain.isEmpty()) {
        if (error) {
            *error = QStringLiteral("no_keychain");
        }
        return false;
    }

    const QByteArray key = keychain.key(entry.cseKey);
    if (key.isEmpty()) {
        if (error) {
            *error = QStringLiteral("missing_key");
        }
        return false;
    }

    QString value;
    if (!decryptField(entry.label, key, &value)) {
        if (error) {
            *error = QStringLiteral("decrypt_failed");
        }
        return false;
    }
    entry.label = value;
    if (!decryptField(entry.username, key, &value)) return false;
    entry.username = value;
    if (!decryptField(entry.password, key, &value)) return false;
    entry.password = value;
    if (!decryptField(entry.url, key, &value)) return false;
    entry.url = value;
    if (!decryptField(entry.notes, key, &value)) return false;
    entry.notes = value;
    if (!decryptField(entry.customFieldsJson, key, &value)) {
        if (error) {
            *error = QStringLiteral("decrypt_failed");
        }
        return false;
    }
    entry.customFieldsJson = value;
    entry.customFields = customFieldsFromJson(value);
    return true;
}

bool decryptFolder(Folder &folder, const Keychain &keychain, QString *error)
{
    if (folder.cseType.isEmpty() || folder.cseType == QLatin1String("none")) {
        return true;
    }
    if (folder.cseType != QLatin1String("CSEv1r1")) {
        if (error) *error = QStringLiteral("unsupported_encryption");
        return false;
    }
    if (keychain.isEmpty()) {
        if (error) *error = QStringLiteral("no_keychain");
        return false;
    }
    const QByteArray key = keychain.key(folder.cseKey);
    if (key.isEmpty()) {
        if (error) *error = QStringLiteral("missing_key");
        return false;
    }
    QString label;
    if (!decryptField(folder.label, key, &label)) {
        if (error) *error = QStringLiteral("decrypt_failed");
        return false;
    }
    folder.label = label;
    return true;
}

bool decryptTag(Tag &tag, const Keychain &keychain, QString *error)
{
    if (tag.cseType.isEmpty() || tag.cseType == QLatin1String("none")) {
        return true;
    }
    if (tag.cseType != QLatin1String("CSEv1r1")) {
        if (error) *error = QStringLiteral("unsupported_encryption");
        return false;
    }
    if (keychain.isEmpty()) {
        if (error) *error = QStringLiteral("no_keychain");
        return false;
    }
    const QByteArray key = keychain.key(tag.cseKey);
    if (key.isEmpty()) {
        if (error) *error = QStringLiteral("missing_key");
        return false;
    }
    QString label;
    QString color;
    if (!decryptField(tag.label, key, &label)
            || !decryptField(tag.color, key, &color)) {
        if (error) *error = QStringLiteral("decrypt_failed");
        return false;
    }
    tag.label = label;
    tag.color = color;
    return true;
}

QJsonObject passwordPayload(const PasswordEntry &entry, const Keychain &keychain)
{
    const bool cse = !keychain.isEmpty();
    const QByteArray key = cse ? keychain.currentKey() : QByteArray();

    QJsonObject object;
    if (!entry.id.isEmpty()) {
        object.insert(QStringLiteral("id"), entry.id);
    }
    if (!entry.revision.isEmpty()) {
        object.insert(QStringLiteral("revision"), entry.revision);
    }

    object.insert(QStringLiteral("label"), encryptField(entry.label, key));
    object.insert(QStringLiteral("username"), encryptField(entry.username, key));
    object.insert(QStringLiteral("password"), encryptField(entry.password, key));
    object.insert(QStringLiteral("url"), encryptField(entry.url, key));
    object.insert(QStringLiteral("notes"), encryptField(entry.notes, key));
    object.insert(QStringLiteral("customFields"), encryptField(customFieldsJson(entry), key));

    object.insert(QStringLiteral("folder"),
                  entry.folderId.isEmpty() ? defaultFolderId() : entry.folderId);
    object.insert(QStringLiteral("cseType"),
                  cse ? QStringLiteral("CSEv1r1") : QStringLiteral("none"));
    object.insert(QStringLiteral("cseKey"), cse ? keychain.currentId() : QString());
    QJsonArray tagIds;
    for (const QString &tagId : entry.tagIds) {
        tagIds.append(tagId);
    }
    object.insert(QStringLiteral("tags"), tagIds);

    object.insert(QStringLiteral("hash"), sha1Hex(entry.password));
    object.insert(QStringLiteral("edited"), static_cast<double>(nowSeconds()));
    object.insert(QStringLiteral("hidden"), entry.hidden);
    object.insert(QStringLiteral("favorite"), entry.favorite);
    return object;
}

QJsonObject folderPayload(const Folder &folder, const Keychain &keychain)
{
    const bool cse = !keychain.isEmpty();
    const QByteArray key = cse ? keychain.currentKey() : QByteArray();

    QJsonObject object;
    if (!folder.id.isEmpty()) {
        object.insert(QStringLiteral("id"), folder.id);
    }
    if (!folder.revision.isEmpty()) {
        object.insert(QStringLiteral("revision"), folder.revision);
    }

    object.insert(QStringLiteral("label"), encryptField(folder.label, key));
    object.insert(QStringLiteral("parent"),
                  folder.parentId.isEmpty() ? defaultFolderId() : folder.parentId);
    object.insert(QStringLiteral("cseType"),
                  cse ? QStringLiteral("CSEv1r1") : QStringLiteral("none"));
    object.insert(QStringLiteral("cseKey"), cse ? keychain.currentId() : QString());
    object.insert(QStringLiteral("edited"), static_cast<double>(nowSeconds()));
    object.insert(QStringLiteral("hidden"), folder.hidden);
    object.insert(QStringLiteral("favorite"), folder.favorite);
    return object;
}

QJsonObject tagPayload(const Tag &tag, const Keychain &keychain)
{
    const bool cse = !keychain.isEmpty();
    const QByteArray key = cse ? keychain.currentKey() : QByteArray();

    QJsonObject object;
    if (!tag.id.isEmpty()) {
        object.insert(QStringLiteral("id"), tag.id);
    }
    if (!tag.revision.isEmpty()) {
        object.insert(QStringLiteral("revision"), tag.revision);
    }

    object.insert(QStringLiteral("label"), encryptField(tag.label, key));
    object.insert(QStringLiteral("color"), encryptField(tag.color, key));
    object.insert(QStringLiteral("cseType"),
                  cse ? QStringLiteral("CSEv1r1") : QStringLiteral("none"));
    object.insert(QStringLiteral("cseKey"), cse ? keychain.currentId() : QString());
    object.insert(QStringLiteral("edited"), static_cast<double>(nowSeconds()));
    object.insert(QStringLiteral("hidden"), tag.hidden);
    object.insert(QStringLiteral("favorite"), tag.favorite);
    return object;
}
