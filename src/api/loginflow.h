#ifndef LOGINFLOW_H
#define LOGINFLOW_H

#include <QString>
#include <functional>

struct LoginFlowStart
{
    bool ok = false;
    int status = 0;
    QString url;
    QString endpoint;
    QString token;
    QString error;
    QString detail;
};

struct LoginFlowResult
{
    bool ok = false;
    bool pending = false;
    int status = 0;
    QString server;
    QString loginName;
    QString appPassword;
    QString error;
    QString detail;
};

namespace LoginFlow {

LoginFlowStart start(const QString &serverUrl);
LoginFlowResult pollOnce(const QString &endpoint, const QString &token);
LoginFlowResult poll(const QString &endpoint,
                     const QString &token,
                     int timeoutSeconds,
                     const std::function<bool()> &cancelled);

} // namespace LoginFlow

#endif // LOGINFLOW_H
