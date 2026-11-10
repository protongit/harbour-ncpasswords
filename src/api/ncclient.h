#ifndef NCCLIENT_H
#define NCCLIENT_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QUrl>

struct ApiResult
{
    int status = 0;
    bool ok = false;
    QJsonDocument json;
    QByteArray body;
    QString error;
    QString detail;
};

class NcClient
{
public:
    NcClient(const QUrl &baseUrl,
             const QString &user,
             const QString &password,
             const QString &sessionId = QString());

    static QUrl apiBase(const QString &serverUrl);

    ApiResult get(const QString &path);
    ApiResult post(const QString &path, const QJsonObject &body = QJsonObject());
    ApiResult post(const QString &path, const QJsonArray &body);
    ApiResult patch(const QString &path, const QJsonObject &body = QJsonObject());
    ApiResult remove(const QString &path, const QJsonObject &body = QJsonObject());

    QString sessionId() const { return m_sessionId; }
    void setSessionId(const QString &sessionId) { m_sessionId = sessionId; }

    QString user() const { return m_user; }

private:
    ApiResult send(const QByteArray &verb, const QString &path, const QJsonDocument &body);

    QUrl m_baseUrl;
    QString m_user;
    QString m_password;
    QString m_sessionId;
};

QString apiErrorCode(const ApiResult &result);

#endif // NCCLIENT_H
