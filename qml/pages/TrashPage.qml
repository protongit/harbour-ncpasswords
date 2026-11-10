import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"

Page {
    id: page
    allowedOrientations: defaultAllowedOrientations

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: vault.trash

        header: PageHeader {
            title: I18n.t("trash_title")
        }

        delegate: ListItem {
            contentHeight: Theme.itemSizeMedium

            onClicked: pageStack.push(Qt.resolvedUrl("EntryPage.qml"), { passwordId: passwordId })

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2 - buttons.width
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.paddingSmall / 2

                Label {
                    width: parent.width
                    text: label
                    color: Theme.primaryColor
                    truncationMode: TruncationMode.Fade
                }

                Label {
                    width: parent.width
                    text: username.length > 0 ? username : url
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    truncationMode: TruncationMode.Fade
                }
            }

            Row {
                id: buttons
                anchors.right: parent.right
                anchors.rightMargin: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter

                IconButton {
                    icon.source: "image://theme/icon-m-forward"
                    onClicked: vault.restorePassword(passwordId)
                }

                IconButton {
                    icon.source: "image://theme/icon-m-delete"
                    onClicked: vault.deletePassword(passwordId)
                }
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: I18n.t("no_trash")
        }

        PullDownMenu {
            MenuItem {
                text: I18n.t("refresh")
                onClicked: vault.reloadTrash()
            }
        }
    }
}
