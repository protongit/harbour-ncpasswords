#include "ncclient.h"

#include <QBuffer>
#include <QEventLoop>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace {

QString normalizedApiPath(const QString &path)
{
    QString result = path;
    while (result.startsWith('/')) {
        result.remove(0, 1);
    }
    return result;
}

} // namespace

NcClient::NcClient(const QUrl &baseUrl,
                   const QString &user,
                   const QString &password,
                   const QString &sessionId)
    : m_baseUrl(baseUrl)
    , m_user(user)
    , m_password(password)
    , m_sessionId(sessionId)
{
}

QUrl NcClient::apiBase(const QString &serverUrl)
{
    QString text = serverUrl.trimmed();
    if (text.isEmpty()) {
        return QUrl();
    }
    if (!text.startsWith(QLatin1String("http://"))
            && !text.startsWith(QLatin1String("https://"))) {
        text.prepend(QLatin1String("https://"));
    }
    while (text.endsWith('/')) {
        text.chop(1);
    }
    if (!text.endsWith(QLatin1String("/index.php/apps/passwords/api"))) {
        text += QLatin1String("/index.php/apps/passwords/api");
    }
    return QUrl(text + QLatin1String("/"));
}

ApiResult NcClient::get(const QString &path)
{
    return send("GET", path, QJsonDocument());
}

ApiResult NcClient::post(const QString &path, const QJsonObject &body)
{
    return send("POST", path, QJsonDocument(body));
}

ApiResult NcClient::post(const QString &path, const QJsonArray &body)
{
    return send("POST", path, QJsonDocument(body));
}

ApiResult NcClient::patch(const QString &path, const QJsonObject &body)
{
    return send("PATCH", path, QJsonDocument(body));
}

ApiResult NcClient::remove(const QString &path, const QJsonObject &body)
{
    return send("DELETE", path, QJsonDocument(body));
}

ApiResult NcClient::send(const QByteArray &verb, const QString &path, const QJsonDocument &body)
{
    ApiResult result;

    if (!m_baseUrl.isValid()) {
        result.error = QStringLiteral("invalid_url");
        return result;
    }

    const QUrl url = m_baseUrl.resolved(QUrl(normalizedApiPath(path)));
    if (!url.isValid()) {
        result.error = QStringLiteral("invalid_url");
        return result;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("harbour-ncpasswords/0.1.0"));
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("OCS-APIRequest", "true");
    request.setRawHeader("Authorization",
                         "Basic " + (m_user + QLatin1Char(':') + m_password).toUtf8().toBase64());
    if (!m_sessionId.isEmpty()) {
        request.setRawHeader("x-api-session", m_sessionId.toUtf8());
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
#else
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif

    const QByteArray data = body.isNull() ? QByteArray()
                                          : body.toJson(QJsonDocument::Compact);

    // Create the event loop (and therefore the thread's event dispatcher) before
    // the network manager. QNetworkAccessManager needs a dispatcher to deliver
    // its internal events, and worker threads only get one with an event loop.
    QEventLoop loop;
    QNetworkAccessManager manager;

    QNetworkReply *reply = 0;
    QBuffer *buffer = 0;

    if (verb == QByteArrayLiteral("GET") || verb == QByteArrayLiteral("POST")) {
        if (verb == QByteArrayLiteral("GET")) {
            reply = manager.get(request);
        } else {
            reply = manager.post(request, data);
        }
    } else {
        if (!data.isEmpty()) {
            buffer = new QBuffer();
            buffer->setData(data);
            buffer->open(QIODevice::ReadOnly);
        }
        reply = manager.sendCustomRequest(request, verb, buffer);
        if (buffer) {
            buffer->setParent(reply);
        }
    }

    if (!reply) {
        result.error = QStringLiteral("network");
        return result;
    }

    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    timeout.start(30000);

    if (!reply->isFinished()) {
        loop.exec();
    }

    if (!reply->isFinished()) {
        result.error = QStringLiteral("timeout");
        reply->abort();
        reply->deleteLater();
        return result;
    }

    result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.body = reply->readAll();

    const QString sessionId = QString::fromUtf8(reply->rawHeader("x-api-session"));
    if (!sessionId.isEmpty()) {
        m_sessionId = sessionId;
    }

    const QByteArray contentType = reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
    if (!result.body.isEmpty()
            && (contentType.contains("json") || result.body.startsWith('{')
                || result.body.startsWith('['))) {
        QJsonParseError parseError;
        result.json = QJsonDocument::fromJson(result.body, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            result.json = QJsonDocument();
        }
    }

    result.ok = result.status >= 200 && result.status < 300;
    if (!result.ok) {
        result.error = apiErrorCode(result);
    }

    const QNetworkReply::NetworkError networkError = reply->error();
    if (networkError != QNetworkReply::NoError && result.status == 0) {
        result.error = QStringLiteral("network");
        result.detail = reply->errorString();
    }

    reply->deleteLater();
    return result;
}

QString apiErrorCode(const ApiResult &result)
{
    switch (result.status) {
    case 0:
        return result.error.isEmpty() ? QStringLiteral("network") : result.error;
    case 400:
        return QStringLiteral("bad_request");
    case 401:
        return QStringLiteral("unauthorized");
    case 403:
        return QStringLiteral("forbidden");
    case 404:
        return QStringLiteral("not_found");
    case 405:
        return QStringLiteral("method_not_allowed");
    case 412:
        return QStringLiteral("conflict");
    case 429:
        return QStringLiteral("rate_limited");
    default:
        if (result.status >= 500) {
            return QStringLiteral("server_error");
        }
        return QStringLiteral("http_error");
    }
}
