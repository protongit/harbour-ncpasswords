import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"
import "../components"

Page {
    id: page
    allowedOrientations: defaultAllowedOrientations

    property bool mismatch: false

    function doApply() {
        mismatch = newPinField.text.length > 0 && newPinField.text !== confirmPinField.text
        if (mismatch) {
            return
        }
        vault.clearError()
        if (vault.pinEnabled) {
            vault.changePin(currentPinField.text, newPinField.text)
        } else {
            vault.enablePin(newPinField.text)
        }
    }

    function doDisable() {
        vault.clearError()
        vault.disablePin(currentPinField.text)
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
                title: I18n.t("pin_lock")
            }

            Label {
                width: parent.width
                text: vault.pinEnabled ? I18n.t("pin_enabled") : I18n.t("pin_disabled")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
            }

            TextField {
                id: currentPinField
                width: parent.width
                visible: vault.pinEnabled
                label: I18n.t("pin_current")
                echoMode: TextInput.Password
                inputMethodHints: Qt.ImhDigitsOnly
                maximumLength: 6
            }

            TextField {
                id: newPinField
                width: parent.width
                label: I18n.t("pin_new")
                echoMode: TextInput.Password
                inputMethodHints: Qt.ImhDigitsOnly
                maximumLength: 6
                onTextChanged: page.mismatch = false
            }

            TextField {
                id: confirmPinField
                width: parent.width
                label: I18n.t("pin_confirm")
                echoMode: TextInput.Password
                inputMethodHints: Qt.ImhDigitsOnly
                maximumLength: 6
                onTextChanged: page.mismatch = false
            }

            Label {
                width: parent.width
                visible: page.mismatch
                text: I18n.t("pin_mismatch")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Button {
                width: parent.width
                text: vault.pinEnabled ? I18n.t("pin_change") : I18n.t("pin_enable")
                onClicked: page.doApply()
            }

            Button {
                width: parent.width
                visible: vault.pinEnabled
                text: I18n.t("pin_disable")
                onClicked: page.doDisable()
            }

            ErrorLabel {
                code: vault.errorCode
                detail: vault.errorDetail
            }
        }
    }

    Connections {
        target: vault
        onPinEnabledChanged: {
            currentPinField.text = ""
            newPinField.text = ""
            confirmPinField.text = ""
            page.mismatch = false
        }
    }
}
