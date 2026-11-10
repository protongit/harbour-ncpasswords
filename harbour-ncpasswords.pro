TARGET = harbour-ncpasswords

CONFIG += sailfishapp c++11 link_pkgconfig
QMAKE_LFLAGS += -rdynamic
QT += quick qml network

PKGCONFIG += sailfishapp libsodium sailfishsecrets

DEFINES += APP_VERSION=\\\"1.0.1\\\"
INCLUDEPATH += src src/crypto src/api src/app src/storage src/cli

SOURCES += \
    src/main.cpp \
    src/crypto/sodiumutils.cpp \
    src/crypto/challenge.cpp \
    src/crypto/keychain.cpp \
    src/crypto/cryptoops.cpp \
    src/crypto/totp.cpp \
    src/api/ncclient.cpp \
    src/api/loginflow.cpp \
    src/api/models.cpp \
    src/app/vaultcontroller.cpp \
    src/app/passwordlistmodel.cpp \
    src/storage/secretstore.cpp \
    src/cli/cli.cpp

HEADERS += \
    src/crypto/sodiumutils.h \
    src/crypto/challenge.h \
    src/crypto/keychain.h \
    src/crypto/cryptoops.h \
    src/crypto/totp.h \
    src/api/ncclient.h \
    src/api/loginflow.h \
    src/api/models.h \
    src/app/vaultcontroller.h \
    src/app/passwordlistmodel.h \
    src/storage/secretstore.h \
    src/cli/cli.h

DISTFILES += \
    qml/harbour-ncpasswords.qml \
    qml/pages/SetupPage.qml \
    qml/pages/UnlockPage.qml \
    qml/pages/VaultPage.qml \
    qml/pages/EntryPage.qml \
    qml/pages/EditPage.qml \
    qml/pages/PinPage.qml \
    qml/pages/PinSettingsPage.qml \
    qml/pages/FolderPage.qml \
    qml/pages/FolderPickerPage.qml \
    qml/pages/FoldersPage.qml \
    qml/pages/TagsPage.qml \
    qml/pages/TrashPage.qml \
    qml/pages/SettingsPage.qml \
    qml/components/FieldRow.qml \
    qml/components/SecretRow.qml \
    qml/cover/CoverPage.qml \
    qml/js/I18n.qml \
    qml/js/qmldir \
    qml/js/locales/en.js \
    qml/js/locales/es.js \
    harbour-ncpasswords.desktop \
    rpm/harbour-ncpasswords.spec

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172 256x256
