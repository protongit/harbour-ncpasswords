#ifndef MODELS_H
#define MODELS_H

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

struct CustomField
{
    QString label;
    QString type;
    QString value;

    QJsonObject toJson() const;
    static CustomField fromJson(const QJsonObject &object);
};

struct PasswordEntry
{
    QString id;
    QString revision;
    QString label;
    QString username;
    QString password;
    QString url;
    QString notes;
    QString customFieldsJson;
    QList<CustomField> customFields;
    QString folderId;
    QString cseType;
    QString cseKey;
    QString sseType;
    QString hash;
    QString statusCode;
    QString client;
    QString share;
    QStringList tagIds;
    int status = 0;
    bool hidden = false;
    bool trashed = false;
    bool favorite = false;
    bool editable = true;
    bool shared = false;
    qint64 edited = 0;
    qint64 created = 0;
    qint64 updated = 0;

    static PasswordEntry fromJson(const QJsonObject &object);
    QJsonObject toJson() const;
};

struct Folder
{
    QString id;
    QString revision;
    QString label;
    QString parentId;
    QString cseType;
    QString cseKey;
    bool hidden = false;
    bool trashed = false;
    bool favorite = false;
    qint64 edited = 0;
    qint64 created = 0;
    qint64 updated = 0;

    static Folder fromJson(const QJsonObject &object);
};

struct Tag
{
    QString id;
    QString revision;
    QString label;
    QString color;
    QString cseType;
    QString cseKey;
    bool hidden = false;
    bool trashed = false;
    bool favorite = false;
    qint64 edited = 0;
    qint64 created = 0;
    qint64 updated = 0;

    static Tag fromJson(const QJsonObject &object);
};

QString defaultFolderId();

// Parses the JSON array stored in the customFields property.
QList<CustomField> customFieldsFromJson(const QString &json);

#endif // MODELS_H
