#include "loginflow.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QTimer>
#include <QUrl>

namespace {

QString normalizeBase(const QString &serverUrl)
{
    QString text = serverUrl.trimmed();
    if (text.isEmpty()) {
        return QString();
    }
    if (!text.startsWith(QLatin1String("http://"))
            && !text.startsWith(QLatin1String("https://"))) {
        text.prepend(QLatin1String("https://"));
    }
    while (text.endsWith('/')) {
        text.chop(1);
    }
    return text;
}

struct RawResponse
{
    int status = 0;
    QByteArray body;
    QString error;
    QString detail;
};

RawResponse post(const QUrl &url,
                 const QByteArray &body,
                 const QByteArray &contentType)
{
    RawResponse result;

    // Create the event loop before the manager so that worker threads have an
    // event dispatcher when QNetworkAccessManager is constructed.
    QEventLoop loop;
    QNetworkAccessManager manager;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("harbour-ncpasswords/0.1.0"));
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("OCS-APIRequest", "true");

    QNetworkReply *reply = manager.post(request, body);

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(30000);
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
    if (result.status == 0) {
        result.error = QStringLiteral("network");
        result.detail = reply->errorString();
    }
    reply->deleteLater();
    return result;
}

} // namespace

namespace LoginFlow {

LoginFlowStart start(const QString &serverUrl)
{
    LoginFlowStart result;

    const QString base = normalizeBase(serverUrl);
    if (base.isEmpty()) {
        result.error = QStringLiteral("invalid_url");
        return result;
    }

    const QUrl url(base + QStringLiteral("/index.php/login/v2"));
    const RawResponse response = post(url, QByteArray(),
                                      QByteArrayLiteral("application/json"));

    result.status = response.status;
    if (response.status != 200) {
        result.error = response.error.isEmpty()
                ? QStringLiteral("login_flow")
                : response.error;
        result.detail = response.detail;
        return result;
    }

    const QJsonDocument document = QJsonDocument::fromJson(response.body);
    if (!document.isObject()) {
        result.error = QStringLiteral("login_flow");
        return result;
    }

    const QJsonObject root = document.object();
    const QJsonObject poll = root.value(QStringLiteral("poll")).toObject();
    result.endpoint = poll.value(QStringLiteral("endpoint")).toString();
    result.token = poll.value(QStringLiteral("token")).toString();
    result.url = root.value(QStringLiteral("login")).toString();

    if (result.endpoint.isEmpty() || result.token.isEmpty() || result.url.isEmpty()) {
        result.error = QStringLiteral("login_flow");
        return result;
    }

    result.ok = true;
    return result;
}

LoginFlowResult pollOnce(const QString &endpoint, const QString &token)
{
    LoginFlowResult result;

    const QUrl url(endpoint);
    const QByteArray body = "token=" + QUrl::toPercentEncoding(token);

    const RawResponse response = post(url, body,
                                      QByteArrayLiteral("application/x-www-form-urlencoded"));
    result.status = response.status;

    if (response.status == 200) {
        const QJsonDocument document = QJsonDocument::fromJson(response.body);
        if (!document.isObject()) {
            result.error = QStringLiteral("login_flow");
            return result;
        }
        const QJsonObject object = document.object();
        result.server = object.value(QStringLiteral("server")).toString();
        result.loginName = object.value(QStringLiteral("loginName")).toString();
        result.appPassword = object.value(QStringLiteral("appPassword")).toString();
        if (result.loginName.isEmpty() || result.appPassword.isEmpty()) {
            result.error = QStringLiteral("login_flow");
            return result;
        }
        result.ok = true;
        return result;
    }

    if (response.status == 404) {
        result.pending = true;
        return result;
    }

    result.error = response.error.isEmpty()
            ? QStringLiteral("login_flow")
            : response.error;
    result.detail = response.detail;
    return result;
}

LoginFlowResult poll(const QString &endpoint,
                     const QString &token,
                     int timeoutSeconds,
                     const std::function<bool()> &cancelled)
{
    const int attempts = qMax(1, timeoutSeconds / 2);
    for (int attempt = 0; attempt < attempts; ++attempt) {
        if (cancelled && cancelled()) {
            LoginFlowResult result;
            result.error = QStringLiteral("cancelled");
            return result;
        }

        const LoginFlowResult result = pollOnce(endpoint, token);
        if (!result.pending) {
            return result;
        }

        QThread::msleep(2000);
    }

    LoginFlowResult result;
    result.error = QStringLiteral("login_flow_timeout");
    return result;
}

} // namespace LoginFlow
