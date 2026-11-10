#include "cli.h"

#include "challenge.h"
#include "cryptoops.h"
#include "keychain.h"
#include "loginflow.h"
#include "models.h"
#include "ncclient.h"
#include "secretstore.h"
#include "sodiumutils.h"
#include "totp.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>

#include <Sailfish/Secrets/collectionnamesrequest.h>
#include <Sailfish/Secrets/deletecollectionrequest.h>
#include <Sailfish/Secrets/result.h>
#include <Sailfish/Secrets/secretmanager.h>

namespace {

QTextStream &out()
{
    static QTextStream stream(stdout);
    return stream;
}

QByteArray bytesInRange(int from, int to)
{
    QByteArray data;
    for (int i = from; i < to; ++i) {
        data.append(static_cast<char>(i));
    }
    return data;
}

void printResult(const QString &name, bool ok, const QString &expected, const QString &actual)
{
    out() << (ok ? "PASS " : "FAIL ") << name << "\n";
    if (!ok) {
        out() << "  expected: " << expected << "\n";
        out() << "  actual  : " << actual << "\n";
    }
    out().flush();
}

} // namespace

namespace Cli {

int selfTest()
{
    if (!Sodium::init()) {
        out() << "FAIL libsodium initialisation\n";
        return 1;
    }

    int failures = 0;

    const QByteArray password = QByteArrayLiteral("correct horse battery staple");
    const QByteArray passwordSalt = bytesInRange(0, 256);
    const QByteArray genericHashKey = bytesInRange(0, 64);
    const QByteArray passwordHashSalt = bytesInRange(16, 32);

    const QString expectedGenericHash =
            QStringLiteral("b121159be7f4d2f0679f53eacca4e5b8173e7ede8f68e41bd4da07351093cd8a"
                           "eb8b096c71dc826be5e41cdb16dad1dfaa14c8244572bb4aef08b34ddcf8193b");
    const QString actualGenericHash = Sodium::toHex(
                Sodium::generichash(password + passwordSalt, genericHashKey, 64));
    printResult(QStringLiteral("genericHash"), actualGenericHash == expectedGenericHash,
                expectedGenericHash, actualGenericHash);
    if (actualGenericHash != expectedGenericHash) ++failures;

    const QString expectedChallenge =
            QStringLiteral("8235948d11e7ef86ecc94b6c5dbf1f162c394e0bde9b8f34d8e24a6c1f559acc");
    const QString actualChallenge = Sodium::toHex(
                Sodium::pwhash(Sodium::fromHex(actualGenericHash), passwordHashSalt, 32));
    printResult(QStringLiteral("challenge"), actualChallenge == expectedChallenge,
                expectedChallenge, actualChallenge);
    if (actualChallenge != expectedChallenge) ++failures;

    const QByteArray key = bytesInRange(0, 32);
    const QByteArray nonce = bytesInRange(64, 88);
    const QByteArray message = QByteArrayLiteral("harbour-ncpasswords");
    const QString expectedBox =
            QStringLiteral("404142434445464748494a4b4c4d4e4f5051525354555657"
                           "1096cb5fd3151318bbcfade3c32af4712276271b5524c907a83a01001bfa2d9ebc5335");
    const QString actualBox = Sodium::toHex(
                nonce + Sodium::secretbox(message, nonce, key));
    printResult(QStringLiteral("secretbox"), actualBox == expectedBox,
                expectedBox, actualBox);
    if (actualBox != expectedBox) ++failures;

    const QByteArray keychainSalt = bytesInRange(32, 48);
    const QByteArray keychainNonce = bytesInRange(88, 112);
    const QByteArray keychainJson =
            QByteArrayLiteral("{\"keys\":{\"k1\":\"000102030405060708090a0b0c0d0e0f"
                              "101112131415161718191a1b1c1d1e1f\"},\"current\":\"k1\"}");
    const QString expectedKeychain =
            QStringLiteral("202122232425262728292a2b2c2d2e2f58595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f"
                           "ae4dcb9b2cbb561adea5910b54782606af1b530551b7fcae0ce28efb0b1a1f14b047053a57098af485e957c4b340f36b84a5ccabde42bebfd2d368d3f866168d407d9ab30ad5220b634f296e599259a203db750d0e0b627d2bca54947514f234422db925faf03a54b85be24455cf18c106");
    const QByteArray keychainKey = Sodium::pwhash(password, keychainSalt, 32);
    const QString actualKeychain = Sodium::toHex(
                keychainSalt + keychainNonce
                + Sodium::secretbox(keychainJson, keychainNonce, keychainKey));
    printResult(QStringLiteral("keychain"), actualKeychain == expectedKeychain,
                expectedKeychain, actualKeychain);
    if (actualKeychain != expectedKeychain) ++failures;

    bool keychainOk = false;
    const Keychain keychain = Keychain::fromEncryptedJson(actualKeychain,
                                                          QString::fromLatin1(password),
                                                          &keychainOk);
    const bool keychainDecryptOk = keychainOk && keychain.currentId() == QStringLiteral("k1")
            && keychain.key(QStringLiteral("k1")) == key;
    printResult(QStringLiteral("keychain decrypt"), keychainDecryptOk,
                QStringLiteral("k1"), keychain.currentId());
    if (!keychainDecryptOk) ++failures;

    TotpConfig totp;
    totp.secret = QStringLiteral("GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ");
    totp.type = QStringLiteral("totp");
    totp.algorithm = QStringLiteral("sha1");
    totp.digits = 8;
    totp.period = 30;
    totp.valid = true;

    struct TotpVector { qint64 time; const char *code; };
    const TotpVector vectors[] = {
        { 59LL, "94287082" },
        { 1111111109LL, "07081804" },
        { 1111111111LL, "14050471" },
        { 1234567890LL, "89005924" },
        { 2000000000LL, "69279037" },
        { 20000000000LL, "65353130" }
    };
    for (const TotpVector &vector : vectors) {
        const QString actual = Totp::code(totp, vector.time);
        const QString expected = QString::fromLatin1(vector.code);
        printResult(QStringLiteral("totp %1").arg(vector.time), actual == expected,
                    expected, actual);
        if (actual != expected) ++failures;
    }

    const TotpConfig parsedTotp = Totp::fromValue(QStringLiteral(
            "{\"secret\":\"GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ\",\"type\":\"totp\","
            "\"algorithm\":\"sha1\",\"digits\":6,\"period\":30}"));
    const QString parsedCode = Totp::code(parsedTotp, 59);
    printResult(QStringLiteral("totp json"), parsedCode == QStringLiteral("287082"),
                QStringLiteral("287082"), parsedCode);
    if (parsedCode != QStringLiteral("287082")) ++failures;

    out() << (failures == 0 ? "ALL TESTS PASSED\n" : "TESTS FAILED\n");
    out().flush();
    return failures == 0 ? 0 : 1;
}

int run(const QStringList &arguments)
{
    // arguments[0] is the executable, arguments[1] is --cli
    if (arguments.size() < 3) {
        out() << "Usage: harbour-ncpasswords --cli <command> ...\n";
        out() << "  probe <server> <user> <password>\n";
        out() << "  list  <server> <user> <password> <master>\n";
        out() << "  create <server> <user> <password> <master> <label> <username> <secret>\n";
        return 1;
    }

    const QString command = arguments.at(2);
    const QString server = arguments.value(3);
    const QString user = arguments.value(4);
    const QString password = arguments.value(5);

    if (command == QLatin1String("secretscollections")) {
        Sailfish::Secrets::SecretManager manager;
        Sailfish::Secrets::CollectionNamesRequest request;
        request.setManager(&manager);
        request.setStoragePluginName(Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
        request.startRequest();
        request.waitForFinished();
        out() << "code=" << static_cast<int>(request.result().code())
              << " error=" << static_cast<int>(request.result().errorCode())
              << " " << request.result().errorMessage() << "\n";
        for (const QString &name : request.collectionNames()) {
            out() << "COLLECTION=" << name << "\n";
        }
        out().flush();
        return request.result().code() == Sailfish::Secrets::Result::Succeeded ? 0 : 18;
    }

    if (command == QLatin1String("secretsdeletecollection")) {
        Sailfish::Secrets::SecretManager manager;
        Sailfish::Secrets::DeleteCollectionRequest request;
        request.setManager(&manager);
        request.setCollectionName(arguments.value(3));
        request.setStoragePluginName(Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
        request.setUserInteractionMode(Sailfish::Secrets::SecretManager::SystemInteraction);
        request.startRequest();
        request.waitForFinished();
        out() << "code=" << static_cast<int>(request.result().code())
              << " error=" << static_cast<int>(request.result().errorCode())
              << " " << request.result().errorMessage() << "\n";
        out().flush();
        return request.result().code() == Sailfish::Secrets::Result::Succeeded ? 0 : 19;
    }

    if (command == QLatin1String("accountload")) {
        QString storedServer;
        QString storedUser;
        QString storedPassword;
        const bool ok = SecretStore::load(&storedServer, &storedUser, &storedPassword);
        out() << "OK=" << (ok ? 1 : 0)
              << " SECRETS=" << (SecretStore::usingSecrets() ? 1 : 0)
              << " SERVER=" << storedServer
              << " USER=" << storedUser
              << " PWLEN=" << storedPassword.size() << "\n";
        out().flush();
        return ok ? 0 : 12;
    }

    if (command == QLatin1String("secretstest")) {
        QString value;
        QString error;
        const bool ok = SecretStore::testSecrets(arguments.value(3), &value, &error);
        out() << (ok ? "SECRETS OK" : "SECRETS FAILED") << "\n";
        if (ok) {
            out() << "VALUE=" << value << "\n";
        } else {
            out() << "ERROR=" << error << "\n";
        }
        out().flush();
        return ok ? 0 : 11;
    }

    if (command == QLatin1String("loginflow-start")) {
        const LoginFlowStart startResult = LoginFlow::start(server);
        if (!startResult.ok) {
            out() << "error: " << startResult.error << "\n";
            out().flush();
            return 9;
        }
        out() << "URL=" << startResult.url << "\n";
        out() << "ENDPOINT=" << startResult.endpoint << "\n";
        out() << "TOKEN=" << startResult.token << "\n";
        out().flush();
        return 0;
    }

    if (command == QLatin1String("loginflow-poll")) {
        const QString endpoint = arguments.value(3);
        const QString token = arguments.value(4);
        const LoginFlowResult pollResult = LoginFlow::poll(endpoint, token, 600, []() {
            return false;
        });
        if (!pollResult.ok) {
            out() << "error: " << pollResult.error << "\n";
            out().flush();
            return 10;
        }
        out() << "LOGIN=" << pollResult.loginName << "\n";
        out() << "APPPASSWORD=" << pollResult.appPassword << "\n";
        out().flush();
        return 0;
    }

    NcClient client(NcClient::apiBase(server), user, password);

    if (command == QLatin1String("probe")) {
        const ApiResult response = client.get(QStringLiteral("1.0/session/request"));
        out() << "HTTP " << response.status << "\n";
        if (response.ok) {
            const QJsonObject challenge = response.json.object()
                    .value(QStringLiteral("challenge")).toObject();
            out() << "challenge salts: "
                  << challenge.value(QStringLiteral("salts")).toArray().size() << "\n";
        } else {
            out() << "error: " << response.error << "\n";
            if (!response.detail.isEmpty()) {
                out() << "detail: " << response.detail << "\n";
            }
        }
        out().flush();
        return response.ok ? 0 : 2;
    }

    if (arguments.size() < 7) {
        out() << "Missing master password\n";
        return 1;
    }
    const QString master = arguments.at(6);

    const ApiResult request = client.get(QStringLiteral("1.0/session/request"));
    if (!request.ok) {
        out() << "session/request failed: " << request.error
              << " (HTTP " << request.status << ")\n";
        return 2;
    }

    QString sessionId;
    Keychain keychain;

    const QJsonObject challengeObject = request.json.object()
            .value(QStringLiteral("challenge")).toObject();
    if (challengeObject.value(QStringLiteral("salts")).toArray().size() == 3) {
        bool challengeOk = false;
        const PwdChallenge challenge = PwdChallenge::fromJson(challengeObject, &challengeOk);
        if (!challengeOk) {
            out() << "Invalid challenge\n";
            return 3;
        }
        const QString solved = challenge.solve(master);
        if (solved.isEmpty()) {
            out() << "Could not solve challenge\n";
            return 4;
        }

        QJsonObject body;
        body.insert(QStringLiteral("challenge"), solved);
        const ApiResult opened = client.post(QStringLiteral("1.0/session/open"), body);
        if (!opened.ok) {
            out() << "session/open failed: " << opened.error
                  << " (HTTP " << opened.status << ")\n";
            return 5;
        }
        sessionId = client.sessionId();

        const QString keychainHex = opened.json.object()
                .value(QStringLiteral("keys")).toObject()
                .value(QStringLiteral("CSEv1r1")).toString();
        bool keychainOk = false;
        keychain = Keychain::fromEncryptedJson(keychainHex, master, &keychainOk);
        if (!keychainOk) {
            out() << "Could not decrypt keychain\n";
            return 6;
        }
    } else {
        out() << "Server does not use client-side encryption\n";
    }

    if (command == QLatin1String("list")) {
        client.setSessionId(sessionId);
        QJsonObject details;
        details.insert(QStringLiteral("details"), QStringLiteral("model+folder+tags"));
        const ApiResult list = client.post(QStringLiteral("1.0/password/list"), details);
        if (!list.ok) {
            out() << "password/list failed: " << list.error << "\n";
            return 7;
        }
        out() << "ID\tLABEL\tUSERNAME\tURL\n";
        for (const QJsonValue &value : list.json.array()) {
            PasswordEntry entry = PasswordEntry::fromJson(value.toObject());
            QString decryptError;
            if (!decryptPassword(entry, keychain, &decryptError)) {
                out() << entry.id << "\t<decrypt failed: " << decryptError << ">\n";
                continue;
            }
            out() << entry.id << '\t' << entry.label << '\t'
                  << entry.username << '\t' << entry.url << "\n";
        }
        out().flush();
        return 0;
    }

    if (command == QLatin1String("create-otp")) {
        if (arguments.size() < 9) {
            out() << "Missing label or secret\n";
            return 1;
        }

        QJsonObject otp;
        otp.insert(QStringLiteral("secret"), arguments.at(8));
        otp.insert(QStringLiteral("type"), QStringLiteral("totp"));
        otp.insert(QStringLiteral("algorithm"), QStringLiteral("sha1"));
        otp.insert(QStringLiteral("digits"), 6);
        otp.insert(QStringLiteral("period"), 30);
        otp.insert(QStringLiteral("counter"), 0);

        CustomField field;
        field.label = QStringLiteral("client.ios.otp");
        field.type = QStringLiteral("secret");
        field.value = QString::fromUtf8(QJsonDocument(otp).toJson(QJsonDocument::Compact));

        PasswordEntry entry;
        entry.label = arguments.at(7);
        entry.username = QStringLiteral("otp");
        entry.password = QStringLiteral("otp");
        entry.folderId = defaultFolderId();
        entry.customFields.append(field);

        client.setSessionId(sessionId);
        const ApiResult created = client.post(QStringLiteral("1.0/password/create"),
                                              passwordPayload(entry, keychain));
        out() << "HTTP " << created.status << "\n";
        if (!created.ok) {
            out() << "create-otp failed: " << created.error << "\n";
            out() << QString::fromUtf8(created.body) << "\n";
            return 16;
        }
        out() << "created: "
              << created.json.object().value(QStringLiteral("id")).toString() << "\n";
        out().flush();
        return 0;
    }

    if (command == QLatin1String("totp")) {
        client.setSessionId(sessionId);
        QJsonObject details;
        details.insert(QStringLiteral("details"), QStringLiteral("model"));
        const ApiResult list = client.post(QStringLiteral("1.0/password/list"), details);
        if (!list.ok) {
            out() << "password/list failed: " << list.error << "\n";
            return 17;
        }

        const qint64 now = QDateTime::currentMSecsSinceEpoch() / 1000;
        for (const QJsonValue &value : list.json.array()) {
            PasswordEntry entry = PasswordEntry::fromJson(value.toObject());
            QString decryptError;
            if (!decryptPassword(entry, keychain, &decryptError)) {
                continue;
            }
            for (const CustomField &field : entry.customFields) {
                const bool candidate = field.label.compare(QStringLiteral("client.ios.otp"),
                                                           Qt::CaseInsensitive) == 0
                        || field.label.contains(QStringLiteral("otp"), Qt::CaseInsensitive);
                if (!candidate) {
                    continue;
                }
                const TotpConfig config = Totp::fromValue(field.value);
                if (!config.valid) {
                    continue;
                }
                out() << entry.label << '\t' << Totp::code(config, now)
                      << '\t' << Totp::remaining(config, now) << "s\n";
            }
        }
        out().flush();
        return 0;
    }

    if (command == QLatin1String("create")) {
        if (arguments.size() < 10) {
            out() << "Missing label, username or secret\n";
            return 1;
        }
        PasswordEntry entry;
        entry.label = arguments.at(7);
        entry.username = arguments.at(8);
        entry.password = arguments.at(9);
        entry.folderId = defaultFolderId();

        client.setSessionId(sessionId);
        const QJsonObject payload = passwordPayload(entry, keychain);
        const ApiResult created = client.post(QStringLiteral("1.0/password/create"), payload);
        out() << "HTTP " << created.status << "\n";
        if (!created.ok) {
            out() << "create failed: " << created.error << "\n";
            out() << QString::fromUtf8(created.body) << "\n";
            return 8;
        }
        out() << "created: " << created.json.object().value(QStringLiteral("id")).toString()
              << "\n";
        out().flush();
        return 0;
    }

    if (command == QLatin1String("update") || command == QLatin1String("delete")) {
        if (arguments.size() < 8) {
            out() << "Missing password id\n";
            return 1;
        }
        const QString id = arguments.at(7);

        client.setSessionId(sessionId);
        QJsonObject details;
        details.insert(QStringLiteral("details"), QStringLiteral("model"));
        const ApiResult list = client.post(QStringLiteral("1.0/password/list"), details);
        if (!list.ok) {
            out() << "password/list failed: " << list.error << "\n";
            return 11;
        }

        PasswordEntry entry;
        for (const QJsonValue &value : list.json.array()) {
            PasswordEntry candidate = PasswordEntry::fromJson(value.toObject());
            if (candidate.id == id) {
                QString decryptError;
                if (!decryptPassword(candidate, keychain, &decryptError)) {
                    out() << "decrypt failed: " << decryptError << "\n";
                    return 12;
                }
                entry = candidate;
                break;
            }
        }
        if (entry.id.isEmpty()) {
            out() << "Password not found: " << id << "\n";
            return 13;
        }

        if (command == QLatin1String("delete")) {
            QJsonObject payload;
            payload.insert(QStringLiteral("id"), entry.id);
            payload.insert(QStringLiteral("revision"), entry.revision);
            const ApiResult deleted = client.remove(QStringLiteral("1.0/password/delete"), payload);
            out() << "HTTP " << deleted.status << "\n";
            return deleted.ok ? 0 : 14;
        }

        if (arguments.size() < 11) {
            out() << "Missing label, username or secret\n";
            return 1;
        }
        entry.label = arguments.at(8);
        entry.username = arguments.at(9);
        entry.password = arguments.at(10);

        const QJsonObject payload = passwordPayload(entry, keychain);
        const ApiResult updated = client.patch(QStringLiteral("1.0/password/update"), payload);
        out() << "HTTP " << updated.status << "\n";
        if (!updated.ok) {
            out() << "update failed: " << updated.error << "\n";
            out() << QString::fromUtf8(updated.body) << "\n";
            return 15;
        }
        return 0;
    }

    out() << "Unknown command: " << command << "\n";
    return 1;
}

} // namespace Cli
