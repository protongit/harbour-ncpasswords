import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"
import "../components"

Page {
    id: page
    objectName: "pin"
    allowedOrientations: defaultAllowedOrientations

    function doUnlock() {
        if (vault.busy) {
            return
        }
        vault.clearError()
        vault.verifyPin(pinField.text)
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

                Icon {
                    anchors.centerIn: parent
                    source: "image://theme/icon-m-screenlock"
                    color: Theme.highlightColor
                }
            }

            Label {
                width: parent.width
                text: I18n.t("pin_prompt")
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeLarge
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            TextField {
                id: pinField
                width: parent.width
                label: I18n.t("pin_current")
                echoMode: TextInput.Password
                inputMethodHints: Qt.ImhDigitsOnly
                maximumLength: 6
                Keys.onReturnPressed: page.doUnlock()
            }

            Button {
                width: parent.width
                text: I18n.t("unlock")
                enabled: !vault.busy
                onClicked: page.doUnlock()
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: vault.busy
                visible: vault.busy
                size: BusyIndicatorSize.Medium
            }

            ErrorLabel {
                code: vault.errorCode
                detail: vault.errorDetail
            }

            Button {
                width: parent.width
                text: I18n.t("other_account")
                enabled: !vault.busy
                onClicked: vault.wipeAccount()
            }
        }
    }

    Component.onCompleted: pinField.forceActiveFocus()
}
