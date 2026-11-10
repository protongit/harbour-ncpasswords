import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"

Page {
    id: page
    allowedOrientations: defaultAllowedOrientations

    function createFolder() {
        dialog.folderId = ""
        nameField.text = ""
        dialog.open()
    }

    function editFolder(id, label) {
        dialog.folderId = id
        nameField.text = label
        dialog.open()
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: vault.folders

        header: PageHeader {
            title: I18n.t("folders_title")
        }

        delegate: ListItem {
            contentHeight: Theme.itemSizeSmall
            onClicked: page.editFolder(modelData.id, modelData.label)

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                text: modelData.label
                color: Theme.primaryColor
                truncationMode: TruncationMode.Fade
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: I18n.t("no_folders")
        }

        PullDownMenu {
            MenuItem {
                text: I18n.t("new_folder")
                onClicked: page.createFolder()
            }
        }
    }

    Dialog {
        id: dialog
        parent: page
        property string folderId

        DialogHeader {
            title: dialog.folderId.length > 0 ? I18n.t("folders_title") : I18n.t("new_folder")
            acceptText: I18n.t("save")
            cancelText: I18n.t("cancel")
        }

        Column {
            width: parent.width
            spacing: Theme.paddingMedium

            TextField {
                id: nameField
                width: parent.width
                label: I18n.t("folder_name")
            }

            Button {
                width: parent.width
                visible: dialog.folderId.length > 0
                text: I18n.t("delete")
                onClicked: {
                    vault.deleteFolder(dialog.folderId)
                    dialog.close()
                }
            }
        }

        onAccepted: {
            if (dialog.folderId.length > 0) {
                vault.updateFolder(dialog.folderId, nameField.text, "")
            } else {
                vault.createFolder(nameField.text, "")
            }
        }
    }
}
