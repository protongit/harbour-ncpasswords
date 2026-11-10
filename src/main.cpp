#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTimer>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>
#include <QScopedPointer>
#include <QStringList>
#include <QtQuick>

#include <cstdio>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <execinfo.h>

#include <sailfishapp.h>

#include "app/vaultcontroller.h"
#include "cli/cli.h"
#include "storage/secretstore.h"

static int g_crashFd = -1;

static void crashHandler(int signalNumber)
{
    if (g_crashFd >= 0) {
        char buffer[64];
        const int length = snprintf(buffer, sizeof(buffer),
                                    "CRASH signal=%d\n", signalNumber);
        if (length > 0) {
            ::write(g_crashFd, buffer, static_cast<size_t>(length));
        }
        void *frames[40];
        const int count = backtrace(frames, 40);
        backtrace_symbols_fd(frames, count, g_crashFd);
    }
    _exit(128 + signalNumber);
}

static void installCrashHandler()
{
    QDir().mkpath(SecretStore::directoryPath());
    const QString path = SecretStore::directoryPath() + QStringLiteral("/debug.log");
    g_crashFd = ::open(path.toUtf8().constData(),
                       O_WRONLY | O_CREAT | O_APPEND, 0600);
    signal(SIGSEGV, crashHandler);
    signal(SIGABRT, crashHandler);
    signal(SIGBUS, crashHandler);
    signal(SIGILL, crashHandler);
}

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName(QStringLiteral("org.hensip"));
    QCoreApplication::setApplicationName(QStringLiteral("harbour-ncpasswords"));
#ifdef APP_VERSION
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
#else
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));
#endif

    bool selfTest = false;
    bool cli = false;
    bool qmlCheck = false;
    bool qmlRun = false;
    for (int i = 1; i < argc; ++i) {
        const QByteArray argument(argv[i]);
        if (argument == QByteArrayLiteral("--selftest")) {
            selfTest = true;
        } else if (argument == QByteArrayLiteral("--cli")) {
            cli = true;
        } else if (argument == QByteArrayLiteral("--qmlcheck")) {
            qmlCheck = true;
        } else if (argument == QByteArrayLiteral("--qmlrun")) {
            qmlRun = true;
        }
    }

    const QByteArray protectedTest = qgetenv("NC_PROTECTED_TEST");
    if (!protectedTest.isEmpty()) {
        QCoreApplication application(argc, argv);
        QString value;
        QString error;
        const bool ok = SecretStore::testProtected(QString::fromUtf8(protectedTest),
                                                   &value, &error);

        const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(directory);
        QFile file(directory + QStringLiteral("/protected-test.log"));
        if (file.open(QIODevice::WriteOnly)) {
            file.write(ok ? "OK\n" : "FAIL\n");
            file.write("value=" + value.toUtf8() + "\n");
            file.write("error=" + error.toUtf8() + "\n");
        }
        return ok ? 0 : 1;
    }

    const QByteArray secretsTest = qgetenv("NC_SECRETS_TEST");
    if (!secretsTest.isEmpty()) {
        QCoreApplication application(argc, argv);
        QString value;
        QString error;
        const bool ok = SecretStore::testSecrets(QString::fromUtf8(secretsTest), &value, &error);

        const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(directory);
        QFile file(directory + QStringLiteral("/secrets-test.log"));
        if (file.open(QIODevice::WriteOnly)) {
            file.write(ok ? "OK\n" : "FAIL\n");
            file.write("value=" + value.toUtf8() + "\n");
            file.write("error=" + error.toUtf8() + "\n");
        }
        return ok ? 0 : 1;
    }

    if (selfTest) {
        QCoreApplication application(argc, argv);
        return Cli::selfTest();
    }

    if (cli) {
        QCoreApplication application(argc, argv);
        return Cli::run(application.arguments());
    }

    if (qmlCheck) {
        qputenv("QT_QPA_PLATFORM", "minimal");
        QScopedPointer<QGuiApplication> application(SailfishApp::application(argc, argv));
        installCrashHandler();
        VaultController vault;

        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("vault"), &vault);

        const QStringList pages = QStringList()
                << QStringLiteral("harbour-ncpasswords.qml")
                << QStringLiteral("pages/SetupPage.qml")
                << QStringLiteral("pages/UnlockPage.qml")
                << QStringLiteral("pages/VaultPage.qml")
                << QStringLiteral("pages/EntryPage.qml")
                << QStringLiteral("pages/EditPage.qml")
                << QStringLiteral("pages/FingerprintPage.qml")
                << QStringLiteral("pages/PinPage.qml")
                << QStringLiteral("pages/PinSettingsPage.qml")
                << QStringLiteral("pages/FolderPage.qml")
                << QStringLiteral("pages/FolderPickerPage.qml")
                << QStringLiteral("pages/FoldersPage.qml")
                << QStringLiteral("pages/TagsPage.qml")
                << QStringLiteral("pages/TrashPage.qml")
                << QStringLiteral("pages/SettingsPage.qml")
                << QStringLiteral("cover/CoverPage.qml")
                << QStringLiteral("components/FieldRow.qml")
                << QStringLiteral("components/ErrorLabel.qml");

        int failures = 0;
        fprintf(stderr, "QML check start\n");
        fflush(stderr);
        for (const QString &page : pages) {
            fprintf(stderr, "QML check: %s\n", qPrintable(page));
            fflush(stderr);
            QQmlComponent component(&engine,
                                    SailfishApp::pathTo(QStringLiteral("qml/") + page));
            QObject *object = component.create();
            if (!object) {
                ++failures;
                fprintf(stderr, "QML FAIL %s\n", qPrintable(page));
                const QList<QQmlError> errors = component.errors();
                for (const QQmlError &error : errors) {
                    fprintf(stderr, "  %s\n", qPrintable(error.toString()));
                }
            } else {
                delete object;
            }
        }
        fprintf(stderr, "QML check: %d failure(s)\n", failures);
        return failures == 0 ? 0 : 1;
    }

    if (qmlRun) {
        qputenv("QT_QPA_PLATFORM", "minimal");
        QScopedPointer<QGuiApplication> application(SailfishApp::application(argc, argv));
        installCrashHandler();
        QScopedPointer<QQuickView> view(SailfishApp::createView());

        VaultController vault;
        view->rootContext()->setContextProperty(QStringLiteral("vault"), &vault);

        view->setSource(SailfishApp::pathTo(QStringLiteral("qml/harbour-ncpasswords.qml")));
        QTimer::singleShot(5000, [&vault]() {
            const QVariantMap totp = vault.currentTotp();
            fprintf(stderr, "NC qmlrun state=%s secrets=%d totpCount=%d code=%s remaining=%s\n",
                    qPrintable(vault.state()), vault.secretsActive() ? 1 : 0,
                    vault.totpCount(),
                    qPrintable(totp.value(QStringLiteral("formattedCode")).toString()),
                    qPrintable(totp.value(QStringLiteral("remaining")).toString()));
            fprintf(stderr, "NC qmlrun server=%s user=%s entries=%d error=%s detail=%s\n",
                    qPrintable(vault.server()), qPrintable(vault.user()),
                    vault.passwords()->entries().size(),
                    qPrintable(vault.errorCode()), qPrintable(vault.errorDetail()));
            for (const PasswordEntry &entry : vault.passwords()->entries()) {
                fprintf(stderr, "NC entry %s fields=%d label=%s\n",
                        qPrintable(entry.id), entry.customFields.size(),
                        qPrintable(entry.label));
                for (const CustomField &field : entry.customFields) {
                    fprintf(stderr, "NC   field %s = %s\n",
                            qPrintable(field.label), qPrintable(field.value.left(80)));
                }
            }
            fflush(stderr);
        });
        QTimer::singleShot(8000, application.data(), &QCoreApplication::quit);
        return application->exec();
    }

    QScopedPointer<QGuiApplication> application(SailfishApp::application(argc, argv));
    installCrashHandler();
    QScopedPointer<QQuickView> view(SailfishApp::createView());

    VaultController vault;
    view->rootContext()->setContextProperty(QStringLiteral("vault"), &vault);

    QObject::connect(application.data(), &QGuiApplication::applicationStateChanged,
                     &vault, [&vault](Qt::ApplicationState state) {
        vault.handleApplicationState(static_cast<int>(state));
    });

    view->setSource(SailfishApp::pathTo(QStringLiteral("qml/harbour-ncpasswords.qml")));
    view->show();

    return application->exec();
}
