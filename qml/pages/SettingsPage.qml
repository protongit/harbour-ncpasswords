import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"
import "../components"

Page {
    id: page
    allowedOrientations: defaultAllowedOrientations

    property var lockValues: [0, 10, 30, 60, 120, 300, 600, 1800]
    property var clipboardValues: [0, 10, 30, 60, 120]
    property var strengthValues: ["low", "medium", "high", "veryhigh"]

    function indexOf(value, values) {
        for (var i = 0; i < values.length; ++i) {
            if (values[i] === value) {
                return i
            }
        }
        return 0
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
                title: I18n.t("settings_title")
            }

            ComboBox {
                id: languageCombo
                width: parent.width
                label: I18n.t("language")
                menu: ContextMenu {
                    Repeater {
                        model: I18n.languageCodes
                        MenuItem {
                            text: I18n.languageName(modelData)
                            onClicked: vault.language = modelData
                        }
                    }
                }

                Component.onCompleted: currentIndex = I18n.languageIndex(vault.language)
            }

            SectionHeader {
                text: I18n.t("account")
            }

            Label {
                width: parent.width
                text: I18n.t("server_label") + ": " + vault.server
                color: Theme.primaryColor
                wrapMode: Text.Wrap
            }

            Label {
                width: parent.width
                text: I18n.t("user_label") + ": " + vault.user
                color: Theme.primaryColor
                wrapMode: Text.Wrap
            }

            Label {
                width: parent.width
                text: I18n.t("app_password_storage") + ": "
                      + (vault.secretsActive ? I18n.t("sailfish_secrets") : I18n.t("local_file"))
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            TextSwitch {
                width: parent.width
                text: I18n.t("group_by_folders")
                checked: vault.viewMode === "folders"
                onCheckedChanged: {
                    if (checked !== (vault.viewMode === "folders")) {
                        vault.viewMode = checked ? "folders" : "list"
                    }
                }
            }

            Label {
                width: parent.width
                text: I18n.t("view_mode_hint")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            ValueButton {
                width: parent.width
                label: I18n.t("pin_lock")
                value: vault.pinEnabled ? I18n.t("pin_enabled") : I18n.t("pin_disabled")
                onClicked: pageStack.push(Qt.resolvedUrl("PinSettingsPage.qml"))
            }

            ComboBox {
                width: parent.width
                label: I18n.t("lock_after")
                currentIndex: page.indexOf(vault.lockSeconds, page.lockValues)
                menu: ContextMenu {
                    MenuItem {
                        text: I18n.duration(0)
                        onClicked: vault.lockSeconds = 0
                    }
                    MenuItem {
                        text: I18n.duration(10)
                        onClicked: vault.lockSeconds = 10
                    }
                    MenuItem {
                        text: I18n.duration(30)
                        onClicked: vault.lockSeconds = 30
                    }
                    MenuItem {
                        text: I18n.duration(60)
                        onClicked: vault.lockSeconds = 60
                    }
                    MenuItem {
                        text: I18n.duration(120)
                        onClicked: vault.lockSeconds = 120
                    }
                    MenuItem {
                        text: I18n.duration(300)
                        onClicked: vault.lockSeconds = 300
                    }
                    MenuItem {
                        text: I18n.duration(600)
                        onClicked: vault.lockSeconds = 600
                    }
                    MenuItem {
                        text: I18n.duration(1800)
                        onClicked: vault.lockSeconds = 1800
                    }
                }
            }

            ComboBox {
                width: parent.width
                label: I18n.t("clipboard_clear")
                currentIndex: page.indexOf(vault.clipboardSeconds, page.clipboardValues)
                menu: ContextMenu {
                    MenuItem {
                        text: I18n.duration(0)
                        onClicked: vault.clipboardSeconds = 0
                    }
                    MenuItem {
                        text: I18n.duration(10)
                        onClicked: vault.clipboardSeconds = 10
                    }
                    MenuItem {
                        text: I18n.duration(30)
                        onClicked: vault.clipboardSeconds = 30
                    }
                    MenuItem {
                        text: I18n.duration(60)
                        onClicked: vault.clipboardSeconds = 60
                    }
                    MenuItem {
                        text: I18n.duration(120)
                        onClicked: vault.clipboardSeconds = 120
                    }
                }
            }

            ComboBox {
                width: parent.width
                label: I18n.t("password_strength")
                currentIndex: page.indexOf(vault.passwordStrength, page.strengthValues)
                menu: ContextMenu {
                    MenuItem {
                        text: I18n.t("strength_low")
                        onClicked: vault.passwordStrength = "low"
                    }
                    MenuItem {
                        text: I18n.t("strength_medium")
                        onClicked: vault.passwordStrength = "medium"
                    }
                    MenuItem {
                        text: I18n.t("strength_high")
                        onClicked: vault.passwordStrength = "high"
                    }
                    MenuItem {
                        text: I18n.t("strength_veryhigh")
                        onClicked: vault.passwordStrength = "veryhigh"
                    }
                }
            }

            Label {
                width: parent.width
                text: I18n.t("password_strength_hint")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: I18n.t("about")
            }

            Label {
                width: parent.width
                text: I18n.t("app_name") + "\n" + I18n.t("version") + " " + Qt.application.version
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Button {
                width: parent.width
                text: I18n.t("wipe")
                onClicked: wipeDialog.open()
            }

            ErrorLabel {
                code: vault.errorCode
                detail: vault.errorDetail
            }
        }
    }

    Dialog {
        id: wipeDialog
        parent: page

        DialogHeader {
            title: I18n.t("wipe")
            acceptText: I18n.t("confirm")
            cancelText: I18n.t("cancel")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - Theme.horizontalPageMargin * 2
            text: I18n.t("wipe_confirm")
            color: Theme.primaryColor
            wrapMode: Text.Wrap
        }

        onAccepted: {
            vault.wipeAccount()
            pageStack.pop()
        }
    }
}
