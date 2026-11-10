#include "models.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

namespace {

QString stringValue(const QJsonObject &object, const char *key, const QString &fallback = QString())
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isString() ? value.toString() : fallback;
}

bool boolValue(const QJsonObject &object, const char *key, bool fallback = false)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isBool() ? value.toBool() : fallback;
}

int intValue(const QJsonObject &object, const char *key, int fallback = 0)
{
    const QJsonValue value = object.value(QLatin1String(key));
    if (value.isDouble()) {
        return value.toInt();
    }
    if (value.isString()) {
        bool ok = false;
        const int parsed = value.toString().toInt(&ok);
        return ok ? parsed : fallback;
    }
    return fallback;
}

qint64 longValue(const QJsonObject &object, const char *key, qint64 fallback = 0)
{
    const QJsonValue value = object.value(QLatin1String(key));
    if (value.isDouble()) {
        return static_cast<qint64>(value.toDouble());
    }
    if (value.isString()) {
        bool ok = false;
        const qint64 parsed = value.toString().toLongLong(&ok);
        return ok ? parsed : fallback;
    }
    return fallback;
}

QString objectId(const QJsonValue &value)
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isObject()) {
        return stringValue(value.toObject(), "id");
    }
    return QString();
}

QList<CustomField> parseCustomFields(const QJsonValue &value)
{
    QList<CustomField> fields;

    QJsonArray array;
    if (value.isString()) {
        const QString text = value.toString();
        if (text.isEmpty()) {
            return fields;
        }
        const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8());
        if (document.isArray()) {
            array = document.array();
        }
    } else if (value.isArray()) {
        array = value.toArray();
    }

    for (const QJsonValue &item : array) {
        if (item.isObject()) {
            fields.append(CustomField::fromJson(item.toObject()));
        }
    }
    return fields;
}

QStringList parseTagIds(const QJsonValue &value)
{
    QStringList ids;
    if (!value.isArray()) {
        return ids;
    }
    const QJsonArray array = value.toArray();
    for (const QJsonValue &item : array) {
        const QString id = objectId(item);
        if (!id.isEmpty()) {
            ids.append(id);
        }
    }
    return ids;
}

} // namespace

QJsonObject CustomField::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("label"), label);
    object.insert(QStringLiteral("type"), type);
    object.insert(QStringLiteral("value"), value);
    return object;
}

CustomField CustomField::fromJson(const QJsonObject &object)
{
    CustomField field;
    field.label = stringValue(object, "label");
    field.type = stringValue(object, "type", QStringLiteral("text"));
    field.value = stringValue(object, "value");
    return field;
}

PasswordEntry PasswordEntry::fromJson(const QJsonObject &object)
{
    PasswordEntry entry;
    entry.id = stringValue(object, "id");
    entry.revision = stringValue(object, "revision");
    entry.label = stringValue(object, "label");
    entry.username = stringValue(object, "username");
    entry.password = stringValue(object, "password");
    entry.url = stringValue(object, "url");
    entry.notes = stringValue(object, "notes");
    entry.folderId = objectId(object.value(QStringLiteral("folder")));
    if (entry.folderId.isEmpty()) {
        entry.folderId = stringValue(object, "folderId", defaultFolderId());
    }
    entry.cseType = stringValue(object, "cseType", QStringLiteral("none"));
    entry.cseKey = stringValue(object, "cseKey");
    entry.sseType = stringValue(object, "sseType");
    entry.hash = stringValue(object, "hash");
    entry.statusCode = stringValue(object, "statusCode");
    entry.client = stringValue(object, "client");
    entry.status = intValue(object, "status");
    entry.hidden = boolValue(object, "hidden");
    entry.trashed = boolValue(object, "trashed");
    entry.favorite = boolValue(object, "favorite");
    entry.editable = boolValue(object, "editable", true);
    entry.shared = boolValue(object, "shared");
    entry.share = stringValue(object, "share");
    entry.edited = longValue(object, "edited");
    entry.created = longValue(object, "created");
    entry.updated = longValue(object, "updated");
    entry.tagIds = parseTagIds(object.value(QStringLiteral("tags")));

    const QJsonValue customFields = object.value(QStringLiteral("customFields"));
    entry.customFieldsJson = customFields.isString() ? customFields.toString() : QString();
    entry.customFields = parseCustomFields(customFields);

    return entry;
}

QJsonObject PasswordEntry::toJson() const
{
    QJsonObject object;
    if (!id.isEmpty()) {
        object.insert(QStringLiteral("id"), id);
    }
    if (!revision.isEmpty()) {
        object.insert(QStringLiteral("revision"), revision);
    }
    object.insert(QStringLiteral("label"), label);
    object.insert(QStringLiteral("username"), username);
    object.insert(QStringLiteral("password"), password);
    object.insert(QStringLiteral("url"), url);
    object.insert(QStringLiteral("notes"), notes);
    object.insert(QStringLiteral("customFields"), customFieldsJson);
    object.insert(QStringLiteral("folder"), folderId);
    object.insert(QStringLiteral("cseType"), cseType);
    object.insert(QStringLiteral("cseKey"), cseKey);
    object.insert(QStringLiteral("hash"), hash);
    object.insert(QStringLiteral("status"), status);
    object.insert(QStringLiteral("statusCode"), statusCode);
    object.insert(QStringLiteral("hidden"), hidden);
    object.insert(QStringLiteral("trashed"), trashed);
    object.insert(QStringLiteral("favorite"), favorite);
    object.insert(QStringLiteral("editable"), editable);
    object.insert(QStringLiteral("edited"), edited);
    object.insert(QStringLiteral("created"), created);
    object.insert(QStringLiteral("updated"), updated);
    return object;
}

Folder Folder::fromJson(const QJsonObject &object)
{
    Folder folder;
    folder.id = stringValue(object, "id");
    folder.revision = stringValue(object, "revision");
    folder.label = stringValue(object, "label");
    folder.parentId = objectId(object.value(QStringLiteral("parent")));
    if (folder.parentId.isEmpty()) {
        folder.parentId = stringValue(object, "parentId", defaultFolderId());
    }
    folder.cseType = stringValue(object, "cseType", QStringLiteral("none"));
    folder.cseKey = stringValue(object, "cseKey");
    folder.hidden = boolValue(object, "hidden");
    folder.trashed = boolValue(object, "trashed");
    folder.favorite = boolValue(object, "favorite");
    folder.edited = longValue(object, "edited");
    folder.created = longValue(object, "created");
    folder.updated = longValue(object, "updated");
    return folder;
}

Tag Tag::fromJson(const QJsonObject &object)
{
    Tag tag;
    tag.id = stringValue(object, "id");
    tag.revision = stringValue(object, "revision");
    tag.label = stringValue(object, "label");
    tag.color = stringValue(object, "color", QStringLiteral("#888888"));
    tag.cseType = stringValue(object, "cseType", QStringLiteral("none"));
    tag.cseKey = stringValue(object, "cseKey");
    tag.hidden = boolValue(object, "hidden");
    tag.trashed = boolValue(object, "trashed");
    tag.favorite = boolValue(object, "favorite");
    tag.edited = longValue(object, "edited");
    tag.created = longValue(object, "created");
    tag.updated = longValue(object, "updated");
    return tag;
}

QString defaultFolderId()
{
    return QStringLiteral("00000000-0000-0000-0000-000000000000");
}

QList<CustomField> customFieldsFromJson(const QString &json)
{
    if (json.isEmpty()) {
        return QList<CustomField>();
    }
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
    if (!document.isArray()) {
        return QList<CustomField>();
    }
    QList<CustomField> fields;
    const QJsonArray array = document.array();
    for (const QJsonValue &item : array) {
        if (item.isObject()) {
            fields.append(CustomField::fromJson(item.toObject()));
        }
    }
    return fields;
}
