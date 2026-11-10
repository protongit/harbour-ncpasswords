import QtQuick 2.2
import Sailfish.Silica 1.0
import "js"
import "pages"

ApplicationWindow {
    id: app

    initialPage: Component { SetupPage { } }
    cover: Qt.resolvedUrl("cover/CoverPage.qml")
    allowedOrientations: defaultAllowedOrientations

    function navigate() {
        var current = pageStack.currentPage ? pageStack.currentPage.objectName : "null"
        if (vault.busy) {
            vault.logDebug("navigate deferred busy")
            return
        }
        vault.logDebug("navigate state=" + vault.state + " fp=" + vault.fingerprintRequired
                       + " current=" + current)

        if (vault.state === "locked" && vault.pinRequired && current !== "pin") {
            pageStack.replace(Qt.resolvedUrl("pages/PinPage.qml"))
        } else if (vault.state === "locked" && vault.fingerprintRequired
                && current !== "fingerprint") {
            pageStack.replace(Qt.resolvedUrl("pages/FingerprintPage.qml"))
        } else if (vault.state === "locked" && !vault.fingerprintRequired
                   && !vault.pinRequired && current !== "unlock") {
            pageStack.replace(Qt.resolvedUrl("pages/UnlockPage.qml"))
        } else if (vault.state === "unlocked" && current !== "vault") {
            pageStack.replace(Qt.resolvedUrl("pages/VaultPage.qml"))
        } else if (vault.state === "setup" && current !== "setup") {
            pageStack.replace(Qt.resolvedUrl("pages/SetupPage.qml"))
        }

    }

    Timer {
        id: startTimer
        interval: 0
        running: true
        onTriggered: vault.restoreAccount()
    }

    Timer {
        id: navTimer
        interval: 0
        onTriggered: app.navigate()
    }

    Connections {
        target: vault

        onStateChanged: navTimer.restart()
        onFingerprintRequiredChanged: navTimer.restart()
        onPinRequiredChanged: navTimer.restart()
        onBusyChanged: {
            if (!vault.busy) {
                navTimer.restart()
            }
        }
    }

    Rectangle {
        id: banner

        property string message

        z: 1000
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: bannerLabel.height + Theme.paddingLarge
        color: "#e6000000"
        opacity: 0
        visible: opacity > 0

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }

        Label {
            id: bannerLabel
            anchors.centerIn: parent
            width: parent.width - Theme.horizontalPageMargin * 2
            text: banner.message
            color: "white"
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: Theme.fontSizeSmall
        }

        Timer {
            id: bannerTimer
            interval: 3500
            onTriggered: banner.opacity = 0
        }

        Connections {
            target: vault

            onToast: {
                banner.message = I18n.t("toast_" + key)
                banner.opacity = 1
                bannerTimer.restart()
            }

            onErrorCodeChanged: {
                if (vault.errorCode.length > 0) {
                    var key = "error_" + vault.errorCode
                    var text = I18n.t(key)
                    if (text === key) {
                        text = I18n.t("error_generic")
                    }
                    banner.message = text
                    banner.opacity = 1
                    bannerTimer.restart()
                }
            }
        }
    }
}
