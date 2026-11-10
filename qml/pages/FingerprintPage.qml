import QtQuick 2.2
import Sailfish.Silica 1.0
import org.nemomobile.devicelock 1.0
import "../js"
import "../components"

Page {
    id: page
    objectName: "fingerprint"
    allowedOrientations: defaultAllowedOrientations

    property bool authInProgress: false

    function startAuth() {
        if (page.status !== PageStatus.Active || page.authInProgress) {
            return
        }
        page.authInProgress = true
        vault.logDebug("device auth request")
        authenticator.requestPermission(
                    I18n.t("fingerprint_prompt"),
                    ({}),
                    Authenticator.Fingerprint | Authenticator.SecurityCode)
    }

    Authenticator {
        id: authenticator

        onPermissionGranted: {
            page.authInProgress = false
            vault.logDebug("device auth granted")
            vault.completeFingerprintAuth()
        }

        onAborted: {
            page.authInProgress = false
            vault.logDebug("device auth aborted")
            vault.fingerprintAuthFailed(I18n.t("error_fingerprint_failed"))
        }
    }

    Timer {
        id: autoAuthTimer
        interval: 350
        onTriggered: page.startAuth()
    }

    Component.onCompleted: autoAuthTimer.start()

    onStatusChanged: {
        if (status === PageStatus.Active) {
            autoAuthTimer.restart()
        } else {
            autoAuthTimer.stop()
            authenticator.cancel()
            page.authInProgress = false
        }
    }

    Component.onDestruction: {
        autoAuthTimer.stop()
        authenticator.cancel()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge * 2

        Column {
            id: column
            x: Theme.horizontalPageMargin
            width: parent.width - Theme.horizontalPageMargin * 2
            spacing: Theme.paddingLarge

            PageHeader {
                interactive: false
                title: I18n.t("app_name")
            }

            Item {
                width: parent.width
                height: Theme.itemSizeExtraLarge * 2

                Image {
                    anchors.centerIn: parent
                    source: "image://theme/graphic-fingerprint"
                    width: Theme.iconSizeLarge
                    height: width
                    fillMode: Image.PreserveAspectFit
                }
            }

            Label {
                width: parent.width
                text: I18n.t("fingerprint_unlock")
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeLarge
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            Label {
                width: parent.width
                text: I18n.t("fingerprint_prompt")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            Button {
                width: parent.width
                text: I18n.t("unlock")
                enabled: !page.authInProgress && !vault.busy
                onClicked: page.startAuth()
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: page.authInProgress || vault.busy
                visible: page.authInProgress || vault.busy
                size: BusyIndicatorSize.Medium
            }

            Label {
                width: parent.width
                visible: page.authInProgress || vault.busy
                text: I18n.t("fingerprint_waiting")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            ErrorLabel {
                code: vault.errorCode
                detail: vault.errorDetail
            }

            Button {
                width: parent.width
                text: I18n.t("other_account")
                enabled: !page.authInProgress && !vault.busy
                onClicked: vault.wipeAccount()
            }
        }
    }
}
