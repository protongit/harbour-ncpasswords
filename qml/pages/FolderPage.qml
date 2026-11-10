import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"

Page {
    id: page

    property string folderId
    property string folderName
    property string lastSearch: ""

    allowedOrientations: defaultAllowedOrientations

    Component.onCompleted: vault.openFolder(folderId)

    onStatusChanged: {
        if (status === PageStatus.Active) {
            vault.openFolder(folderId)
            if (page.lastSearch.length > 0) {
                vault.setFolderSearch(page.lastSearch)
            }
        }
    }

    Component.onDestruction: vault.closeFolder()

    SilicaListView {
        id: listView
        anchors.fill: parent
        model: vault.folderPasswords

        header: Column {
            width: listView.width

            Item {
                width: parent.width
                height: Theme.itemSizeLarge + Theme.paddingLarge

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - Theme.horizontalPageMargin * 2
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: Theme.paddingMedium
                    text: page.folderName.length > 0
                          ? page.folderName
                          : I18n.t("folder_none")
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeLarge
                    font.family: Theme.fontFamilyHeading
                    elide: Text.ElideRight
                }
            }

            SearchField {
                id: searchField
                width: parent.width
                placeholderText: I18n.t("search")

                property bool ready: false
                property bool restoring: false

                Component.onCompleted: {
                    if (page.lastSearch.length > 0) {
                        restoring = true
                        text = page.lastSearch
                        restoring = false
                    }
                    ready = true
                }
                onTextChanged: {
                    if (!ready || restoring) {
                        return
                    }
                    if (text.length === 0) {
                        page.lastSearch = ""
                        vault.setFolderSearch("")
                    }
                }
                Keys.onReturnPressed: {
                    page.lastSearch = text
                    focus = false
                    vault.setFolderSearch(page.lastSearch)
                }
                EnterKey.enabled: text.length > 0
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: {
                    page.lastSearch = text
                    focus = false
                    vault.setFolderSearch(page.lastSearch)
                }
            }

            Item {
                width: 1
                height: Theme.paddingMedium
            }
        }

        delegate: ListItem {
            id: item
            contentHeight: Theme.itemSizeMedium

            onClicked: {
                vault.noteActivity()
                pageStack.push(Qt.resolvedUrl("EntryPage.qml"), { passwordId: passwordId })
            }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.paddingSmall / 2

                Label {
                    width: parent.width - Theme.paddingLarge
                    text: label
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeMedium
                    truncationMode: TruncationMode.Fade
                }

                Label {
                    width: parent.width - Theme.paddingLarge
                    text: username.length > 0 ? username : url
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    truncationMode: TruncationMode.Fade
                }
            }

            Icon {
                visible: favorite
                source: "image://theme/icon-s-favorite"
                anchors.right: parent.right
                anchors.rightMargin: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        ViewPlaceholder {
            enabled: listView.count === 0
            text: page.lastSearch.length > 0
                  ? I18n.t("no_results")
                  : I18n.t("no_passwords")
        }

        PullDownMenu {
            MenuItem {
                text: I18n.t("trash")
                onClicked: pageStack.push(Qt.resolvedUrl("TrashPage.qml"))
            }
            MenuItem {
                text: I18n.t("folders")
                onClicked: pageStack.push(Qt.resolvedUrl("FoldersPage.qml"))
            }
            MenuItem {
                text: I18n.t("tags")
                onClicked: pageStack.push(Qt.resolvedUrl("TagsPage.qml"))
            }
            MenuItem {
                text: I18n.t("settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: I18n.t("refresh")
                onClicked: {
                    vault.refresh()
                    if (page.lastSearch.length > 0) {
                        vault.setFolderSearch(page.lastSearch)
                    }
                }
            }
            MenuItem {
                text: I18n.t("new_password")
                onClicked: pageStack.push(Qt.resolvedUrl("EditPage.qml"),
                                          { passwordId: "", folderId: page.folderId })
            }
            MenuItem {
                text: I18n.t("lock")
                onClicked: vault.lock()
            }
        }

        onMovementStarted: vault.noteActivity()
    }
}
