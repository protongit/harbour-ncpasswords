import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"

Page {
    id: page

    property var target
    property string selectedId

    allowedOrientations: defaultAllowedOrientations

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: vault.folders

        header: Column {
            width: listView.width

            PageHeader {
                interactive: false
                title: I18n.t("folder_label")
            }

            ListItem {
                contentHeight: Theme.itemSizeSmall
                onClicked: {
                    if (target) {
                        target.setFolder("00000000-0000-0000-0000-000000000000")
                    }
                    pageStack.pop()
                }

                Label {
                    x: Theme.horizontalPageMargin
                    text: I18n.t("folder_none")
                    color: Theme.primaryColor
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        delegate: ListItem {
            contentHeight: Theme.itemSizeSmall
            onClicked: {
                if (target) {
                    target.setFolder(modelData.id)
                }
                pageStack.pop()
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                text: modelData.label
                color: modelData.id === selectedId ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
