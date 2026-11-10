import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"

Page {
    id: page
    allowedOrientations: defaultAllowedOrientations

    function createTag() {
        dialog.tagId = ""
        nameField.text = ""
        colorField.text = "#888888"
        dialog.open()
    }

    function editTag(id, label, color) {
        dialog.tagId = id
        nameField.text = label
        colorField.text = color
        dialog.open()
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: vault.tags

        header: PageHeader {
            title: I18n.t("tags_title")
        }

        delegate: ListItem {
            contentHeight: Theme.itemSizeSmall
            onClicked: page.editTag(modelData.id, modelData.label, modelData.color)

            Rectangle {
                x: Theme.horizontalPageMargin
                width: Theme.paddingMedium
                height: Theme.paddingMedium
                radius: height / 2
                color: modelData.color
                anchors.verticalCenter: parent.verticalCenter
            }

            Label {
                x: Theme.horizontalPageMargin + Theme.paddingMedium * 2
                width: parent.width - Theme.horizontalPageMargin * 2 - Theme.paddingMedium * 2
                text: modelData.label
                color: Theme.primaryColor
                truncationMode: TruncationMode.Fade
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: I18n.t("no_tags")
        }

        PullDownMenu {
            MenuItem {
                text: I18n.t("new_tag")
                onClicked: page.createTag()
            }
        }
    }

    Dialog {
        id: dialog
        parent: page
        property string tagId

        DialogHeader {
            title: dialog.tagId.length > 0 ? I18n.t("tags_title") : I18n.t("new_tag")
            acceptText: I18n.t("save")
            cancelText: I18n.t("cancel")
        }

        Column {
            width: parent.width
            spacing: Theme.paddingMedium

            TextField {
                id: nameField
                width: parent.width
                label: I18n.t("tag_name")
            }

            TextField {
                id: colorField
                width: parent.width
                label: I18n.t("color")
                placeholderText: "#888888"
            }

            Button {
                width: parent.width
                visible: dialog.tagId.length > 0
                text: I18n.t("delete")
                onClicked: {
                    vault.deleteTag(dialog.tagId)
                    dialog.close()
                }
            }
        }

        onAccepted: {
            if (dialog.tagId.length > 0) {
                vault.updateTag(dialog.tagId, nameField.text, colorField.text)
            } else {
                vault.createTag(nameField.text, colorField.text)
            }
        }
    }
}
