import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"
import "../components"

Page {
    id: page
    objectName: "unlock"
    allowedOrientations: defaultAllowedOrientations

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
                title: I18n.t("unlock_title")
            }

            Label {
                width: parent.width
                text: vault.user + " \u00b7 " + vault.server
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                elide: Text.ElideRight
            }

            Label {
                width: parent.width
                visible: vault.cseEnabled
                text: I18n.t("unlock_intro")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            TextField {
                id: masterField
                width: parent.width
                visible: vault.cseEnabled
                label: I18n.t("master_password")
                echoMode: TextInput.Password
                inputMethodHints: Qt.ImhNoAutoUppercase
                Keys.onReturnPressed: page.doUnlock()
            }

            Label {
                width: parent.width
                visible: vault.cseEnabled
                text: I18n.t("master_hint")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }

            Button {
                width: parent.width
                text: vault.busy ? I18n.t("unlocking") : I18n.t("unlock")
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

    function doUnlock() {
        if (vault.busy) {
            return
        }
        vault.clearError()
        vault.unlock(masterField.text)
    }

    Component.onCompleted: {
        masterField.forceActiveFocus()
    }
}
