import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"
import "../components"

Page {
    id: page
    objectName: "setup"
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
                title: I18n.t("setup_title")
            }

            Label {
                width: parent.width
                text: I18n.t("setup_intro")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            TextField {
                id: serverField
                width: parent.width
                label: I18n.t("server")
                placeholderText: I18n.t("server_placeholder")
                inputMethodHints: Qt.ImhNoAutoUppercase
            }

            TextField {
                id: userField
                width: parent.width
                label: I18n.t("username")
                inputMethodHints: Qt.ImhNoAutoUppercase
            }

            TextField {
                id: passwordField
                width: parent.width
                label: I18n.t("app_password")
                echoMode: TextInput.Password
                inputMethodHints: Qt.ImhNoAutoUppercase
                Keys.onReturnPressed: page.doConnect()
            }

            Button {
                width: parent.width
                text: vault.busy ? I18n.t("connecting") : I18n.t("connect")
                enabled: !vault.busy && !vault.loginFlowActive
                onClicked: page.doConnect()
            }

            Button {
                width: parent.width
                visible: !vault.loginFlowActive
                text: I18n.t("browser_login")
                enabled: !vault.busy
                onClicked: vault.startLoginFlow(serverField.text)
            }

            Label {
                width: parent.width
                visible: !vault.loginFlowActive
                text: I18n.t("browser_login_hint")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            Label {
                width: parent.width
                visible: vault.loginFlowActive
                text: I18n.t("waiting_browser")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                width: parent.width
                visible: vault.loginFlowActive
                text: I18n.t("cancel_login")
                onClicked: vault.cancelLoginFlow()
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
        }
    }

    function doConnect() {
        if (vault.busy) {
            return
        }
        vault.clearError()
        vault.connectToServer(serverField.text, userField.text, passwordField.text)
    }

    Component.onCompleted: {
        if (vault.server.length > 0) {
            serverField.text = vault.server
        }
        if (vault.user.length > 0) {
            userField.text = vault.user
        }
    }

    Connections {
        target: vault

        onLoginFlowUrl: Qt.openUrlExternally(url)

        onAccountChanged: {
            if (serverField.text.length === 0) {
                serverField.text = vault.server
            }
            if (userField.text.length === 0) {
                userField.text = vault.user
            }
        }
    }
}
