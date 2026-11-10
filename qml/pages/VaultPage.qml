import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"

Page {
    id: page
    objectName: "vault"
    allowedOrientations: defaultAllowedOrientations

    property bool folderMode: vault.viewMode === "folders" && vault.folderFilter.length === 0

    Component.onCompleted: vault.logDebug("VaultPage loaded")

    Connections {
        target: vault.passwords
        onCountChanged: vault.logDebug("password count=" + vault.passwords.count)
    }

    SilicaListView {
        id: listView
        anchors.fill: parent
        visible: !page.folderMode
        model: vault.passwords

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
                    text: I18n.t("vault_title")
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

                // The list header is recreated when the model is reset after
                // filtering. Restore the active text without treating it as
                // a user edit, so the filter is not cleared.
                property bool ready: false
                property bool restoring: false

                Component.onCompleted: {
                    if (vault.searchText.length > 0) {
                        restoring = true
                        text = vault.searchText
                        restoring = false
                    }
                    ready = true
                }
                onTextChanged: {
                    if (!ready || restoring) {
                        return
                    }
                    if (text.length === 0) {
                        vault.setSearch("")
                    }
                }
                Keys.onReturnPressed: {
                    focus = false
                    vault.setSearch(text)
                }
                EnterKey.enabled: text.length > 0
                EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                EnterKey.onClicked: {
                    focus = false
                    vault.setSearch(text)
                }
            }

            Row {
                width: parent.width
                visible: vault.folderFilter.length > 0 || vault.tagFilter.length > 0

                Label {
                    width: parent.width - clearButton.width
                    text: vault.folderFilter.length > 0
                          ? (vault.folderLabel(vault.folderFilter).length > 0
                             ? vault.folderLabel(vault.folderFilter)
                             : I18n.t("folder_none"))
                          : vault.tagLabel(vault.tagFilter)
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    truncationMode: TruncationMode.Fade
                    anchors.verticalCenter: parent.verticalCenter
                }

                Button {
                    id: clearButton
                    text: I18n.t("clear_filter")
                    onClicked: {
                        vault.clearFilters()
                        searchField.text = ""
                    }
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

                Label {
                    width: parent.width - Theme.paddingLarge
                    visible: folderLabel.length > 0
                    text: folderLabel
                    color: Theme.tertiaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
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
            text: vault.searchText.length > 0 ? I18n.t("no_results") : I18n.t("no_passwords")
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
                onClicked: vault.refresh()
            }
            MenuItem {
                text: I18n.t("new_password")
                onClicked: pageStack.push(Qt.resolvedUrl("EditPage.qml"), { passwordId: "" })
            }
            MenuItem {
                text: I18n.t("lock")
                onClicked: vault.lock()
            }
        }

        onMovementStarted: vault.noteActivity()
    }

    SilicaListView {
        id: folderListView
        anchors.fill: parent
        visible: page.folderMode
        model: vault.folders

        header: Column {
            width: folderListView.width

            Item {
                width: parent.width
                height: Theme.itemSizeLarge + Theme.paddingLarge

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - Theme.horizontalPageMargin * 2
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: Theme.paddingMedium
                    text: I18n.t("folders_title")
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeLarge
                    font.family: Theme.fontFamilyHeading
                    elide: Text.ElideRight
                }
            }

            ListItem {
                contentHeight: Theme.itemSizeSmall
                onClicked: pageStack.push(Qt.resolvedUrl("FolderPage.qml"),
                                          { folderId: "00000000-0000-0000-0000-000000000000",
                                            folderName: I18n.t("folder_none") })

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - Theme.horizontalPageMargin * 2
                    text: I18n.t("folder_none")
                    color: Theme.primaryColor
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Item {
                width: 1
                height: Theme.paddingMedium
            }
        }

        delegate: ListItem {
            contentHeight: Theme.itemSizeSmall
            onClicked: pageStack.push(Qt.resolvedUrl("FolderPage.qml"),
                                      { folderId: modelData.id, folderName: modelData.label })

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - Theme.horizontalPageMargin * 2 - Theme.paddingLarge
                text: modelData.label
                color: Theme.primaryColor
                truncationMode: TruncationMode.Fade
                anchors.verticalCenter: parent.verticalCenter
            }

            Icon {
                source: "image://theme/icon-m-forward"
                anchors.right: parent.right
                anchors.rightMargin: Theme.horizontalPageMargin
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        ViewPlaceholder {
            enabled: folderListView.count === 0
            text: I18n.t("no_folders")
        }

        PullDownMenu {
            MenuItem {
                text: I18n.t("view_list")
                onClicked: vault.viewMode = "list"
            }
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
                onClicked: vault.refresh()
            }
            MenuItem {
                text: I18n.t("new_password")
                onClicked: pageStack.push(Qt.resolvedUrl("EditPage.qml"), { passwordId: "" })
            }
            MenuItem {
                text: I18n.t("lock")
                onClicked: vault.lock()
            }
        }

        onMovementStarted: vault.noteActivity()
    }

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.25
        visible: vault.busy
        z: 10
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: vault.busy
        visible: vault.busy
        size: BusyIndicatorSize.Large
        z: 11
    }
}
