import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"
import "../components"

Page {
    id: page

    property string passwordId
    property var entry: ({})
    property bool entryLoaded: false
    property bool showPassword: false
    property var entryTotp: ({})

    allowedOrientations: defaultAllowedOrientations

    function reload() {
        entry = vault.password(passwordId)
        entryLoaded = entry.id !== undefined && entry.id.length > 0
        showPassword = false
        refreshTotp()
    }

    function refreshTotp() {
        entryTotp = vault.totpForPassword(passwordId)
    }

    Component.onCompleted: reload()
    onStatusChanged: {
        if (status === PageStatus.Active) {
            reload()
        }
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
                title: entryLoaded && entry.label.length > 0
                       ? entry.label
                       : I18n.t("detail_title")
            }

            FieldRow {
                caption: I18n.t("username_label")
                value: entryLoaded ? entry.username : ""
            }

            Column {
                width: parent.width
                spacing: Theme.paddingSmall

                Label {
                    text: I18n.t("password_label")
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                }

                Row {
                    width: parent.width
                    spacing: Theme.paddingSmall

                    Label {
                        width: parent.width - showPasswordButton.width
                               - copyPasswordButton.width - Theme.paddingSmall * 2
                        text: page.showPassword
                              ? (entryLoaded ? entry.password : "")
                              : "\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022"
                        color: Theme.primaryColor
                        font.pixelSize: Theme.fontSizeMedium
                        wrapMode: Text.Wrap
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    IconButton {
                        id: showPasswordButton
                        icon.source: page.showPassword
                                     ? "image://theme/icon-splus-hide-password"
                                     : "image://theme/icon-splus-show-password"
                        onClicked: page.showPassword = !page.showPassword
                    }

                    IconButton {
                        id: copyPasswordButton
                        icon.source: "image://theme/icon-m-clipboard"
                        onClicked: vault.copySecret(entry.password)
                    }
                }
            }

            Column {
                width: parent.width
                spacing: Theme.paddingSmall
                visible: entryTotp.code !== undefined && entryTotp.code.length > 0

                Label {
                    text: I18n.t("totp")
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                }

                Row {
                    spacing: Theme.paddingSmall

                    Label {
                        text: entryTotp.formattedCode !== undefined
                              ? entryTotp.formattedCode
                              : ""
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeExtraLarge
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    IconButton {
                        icon.source: "image://theme/icon-m-clipboard"
                        onClicked: vault.copySecret(entryTotp.code)
                    }

                    Label {
                        text: (entryTotp.remaining !== undefined ? entryTotp.remaining : 0) + "s"
                        color: Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeSmall
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            FieldRow {
                caption: I18n.t("url_label")
                value: entryLoaded ? entry.url : ""
                actionIcon: "image://theme/icon-m-website"
                actionVisible: entryLoaded && entry.url.length > 0
                onActionClicked: Qt.openUrlExternally(entry.url)
            }

            FieldRow {
                caption: I18n.t("folder_label")
                value: entryLoaded
                       ? (entry.folderLabel.length > 0 ? entry.folderLabel : I18n.t("folder_none"))
                       : ""
                copyable: false
            }

            Column {
                width: parent.width
                spacing: Theme.paddingSmall
                visible: entryLoaded && entry.tagIds.length > 0

                Label {
                    text: I18n.t("tags_label")
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                }

                Flow {
                    width: parent.width
                    spacing: Theme.paddingSmall
                    Repeater {
                        model: entryLoaded ? entry.tagIds : []
                        Label {
                            text: vault.tagLabel(modelData)
                            color: Theme.highlightColor
                            font.pixelSize: Theme.fontSizeSmall
                        }
                    }
                }
            }

            FieldRow {
                caption: I18n.t("notes_label")
                value: entryLoaded ? entry.notes : ""
                copyable: true
            }

            Column {
                width: parent.width
                spacing: Theme.paddingMedium
                visible: entryLoaded && entry.customFields.length > 0

                SectionHeader {
                    text: I18n.t("custom_fields")
                }

                Repeater {
                    model: entryLoaded ? entry.customFields : []
                    FieldRow {
                        caption: modelData.label
                        value: modelData.value
                        secret: modelData.type === "secret"
                    }
                }
            }

            ErrorLabel {
                code: vault.errorCode
                detail: vault.errorDetail
            }
        }
    }

    Timer {
        interval: 1000
        repeat: true
        running: entryLoaded && page.status === PageStatus.Active
        onTriggered: page.refreshTotp()
    }

    PullDownMenu {
        MenuItem {
            text: I18n.t("edit")
            onClicked: pageStack.push(Qt.resolvedUrl("EditPage.qml"), { passwordId: page.passwordId })
        }
        MenuItem {
            text: entryLoaded && entry.favorite ? I18n.t("favorite") : I18n.t("favorite")
            onClicked: {
                var data = vault.password(page.passwordId)
                if (data.id !== undefined) {
                    data.favorite = !data.favorite
                    vault.updatePassword(data)
                }
            }
        }
        MenuItem {
            text: I18n.t("delete")
            onClicked: deleteDialog.open()
        }
    }

    Dialog {
        id: deleteDialog
        parent: page

        DialogHeader {
            title: I18n.t("delete_confirm_title")
            acceptText: I18n.t("delete")
            cancelText: I18n.t("cancel")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - Theme.horizontalPageMargin * 2
            text: I18n.t("delete_confirm_text")
            wrapMode: Text.Wrap
            color: Theme.primaryColor
        }

        onAccepted: {
            vault.deletePassword(page.passwordId)
            pageStack.pop()
        }
    }
}
