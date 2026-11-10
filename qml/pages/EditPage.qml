import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"
import "../components"

Page {
    id: page

    property string passwordId
    property bool isNew: passwordId.length === 0
    property var entry: ({})
    property string folderId: "00000000-0000-0000-0000-000000000000"
    property string folderName: I18n.t("folder_none")
    property var selectedTagIds: []
    property bool passwordVisible: false

    allowedOrientations: defaultAllowedOrientations

    function load() {
        if (isNew) {
            return
        }
        entry = vault.password(passwordId)
        if (entry.id === undefined) {
            return
        }
        labelField.text = entry.label
        usernameField.text = entry.username
        passwordField.text = entry.password
        urlField.text = entry.url
        notesArea.text = entry.notes
        favoriteSwitch.checked = entry.favorite
        hiddenSwitch.checked = entry.hidden
        setFolder(entry.folderId)

        var tags = []
        if (entry.tagIds !== undefined) {
            for (var t = 0; t < entry.tagIds.length; ++t) {
                tags.push(entry.tagIds[t])
            }
        }
        selectedTagIds = tags

        fieldsModel.clear()
        for (var i = 0; i < entry.customFields.length; ++i) {
            var field = entry.customFields[i]
            fieldsModel.append({ label: field.label, type: field.type, value: field.value })
        }
    }

    function setFolder(id) {
        folderId = id && id.length > 0 ? id : "00000000-0000-0000-0000-000000000000"
        folderName = folderId === "00000000-0000-0000-0000-000000000000"
                ? I18n.t("folder_none")
                : vault.folderLabel(folderId)
    }

    function toggleTag(id, checked) {
        var list = selectedTagIds.slice()
        var index = list.indexOf(id)
        if (checked && index < 0) {
            list.push(id)
        } else if (!checked && index >= 0) {
            list.splice(index, 1)
        }
        selectedTagIds = list
    }

    function save() {
        var custom = []
        for (var i = 0; i < fieldsModel.count; ++i) {
            var field = fieldsModel.get(i)
            if (field.label.length > 0) {
                custom.push({ label: field.label, type: field.type, value: field.value })
            }
        }

        var data = {
            id: isNew ? "" : passwordId,
            label: labelField.text,
            username: usernameField.text,
            password: passwordField.text,
            url: urlField.text,
            notes: notesArea.text,
            folderId: folderId,
            favorite: favoriteSwitch.checked,
            hidden: hiddenSwitch.checked,
            tagIds: selectedTagIds,
            customFields: custom
        }

        vault.clearError()
        if (isNew) {
            vault.createPassword(data)
        } else {
            vault.updatePassword(data)
        }
        pageStack.pop()
    }

    function typeIndex(type) {
        if (type === "secret") return 1
        if (type === "url") return 2
        if (type === "email") return 3
        return 0
    }

    Component.onCompleted: {
        if (isNew) {
            if (folderId.length > 0) {
                setFolder(folderId)
            } else {
                setFolder("00000000-0000-0000-0000-000000000000")
            }
        } else {
            load()
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
                title: isNew ? I18n.t("new_title") : I18n.t("edit_title")
            }

            TextField {
                id: labelField
                width: parent.width
                label: I18n.t("label")
                placeholderText: I18n.t("label_placeholder")
            }

            TextField {
                id: usernameField
                width: parent.width
                label: I18n.t("username_label")
                inputMethodHints: Qt.ImhNoAutoUppercase
            }

            Item {
                width: parent.width
                height: passwordField.height

                TextField {
                    id: passwordField
                    width: parent.width - Theme.itemSizeSmall * 3 - Theme.paddingSmall
                    label: I18n.t("password_label")
                    echoMode: page.passwordVisible ? TextInput.Normal : TextInput.Password
                }

                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 0

                    IconButton {
                        icon.source: "image://theme/icon-m-refresh"
                        onClicked: passwordField.text = vault.generatePasswordForStrength()
                    }

                    IconButton {
                        icon.source: page.passwordVisible
                                    ? "image://theme/icon-splus-hide-password"
                                    : "image://theme/icon-splus-show-password"
                        onClicked: page.passwordVisible = !page.passwordVisible
                    }

                    IconButton {
                        icon.source: "image://theme/icon-m-clipboard"
                        enabled: passwordField.text.length > 0
                        onClicked: vault.copySecret(passwordField.text)
                    }
                }
            }

            TextField {
                id: urlField
                width: parent.width
                label: I18n.t("url_label")
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
            }

            ValueButton {
                width: parent.width
                label: I18n.t("folder_label")
                value: folderName
                onClicked: pageStack.push(Qt.resolvedUrl("FolderPickerPage.qml"),
                                          { target: page, selectedId: folderId })
            }

            Column {
                width: parent.width
                spacing: Theme.paddingSmall

                SectionHeader {
                    text: I18n.t("tags_label")
                }

                Flow {
                    width: parent.width
                    spacing: Theme.paddingSmall

                    Repeater {
                        model: vault.tags

                        Button {
                            width: Math.min(implicitWidth, parent.width)
                            height: implicitHeight
                            text: modelData.label
                            highlighted: page.selectedTagIds.indexOf(modelData.id) >= 0
                            onClicked: page.toggleTag(modelData.id, !highlighted)
                        }
                    }
                }

                Label {
                    width: parent.width
                    visible: vault.tags.length === 0
                    text: I18n.t("no_tags")
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                }
            }

            Column {
                width: parent.width
                spacing: Theme.paddingSmall

                Label {
                    text: I18n.t("favorite")
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                }

                Switch {
                    id: favoriteSwitch
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            Column {
                width: parent.width
                spacing: Theme.paddingSmall

                Label {
                    text: I18n.t("hidden")
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                }

                Switch {
                    id: hiddenSwitch
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            TextArea {
                id: notesArea
                width: parent.width
                label: I18n.t("notes_label")
                height: Theme.itemSizeLarge * 2
            }

            SectionHeader {
                text: I18n.t("custom_fields")
            }

            ListModel {
                id: fieldsModel
            }

            Repeater {
                model: fieldsModel

                Column {
                    width: parent.width
                    spacing: Theme.paddingSmall

                    Row {
                        width: parent.width
                        spacing: Theme.paddingSmall

                        TextField {
                            width: parent.width - typeCombo.width - removeButton.width
                                   - Theme.paddingSmall * 2
                            label: I18n.t("field_label")
                            text: label
                            onTextChanged: fieldsModel.setProperty(index, "label", text)
                        }

                        ComboBox {
                            id: typeCombo
                            width: Theme.itemSizeLarge * 1.6
                            label: I18n.t("field_type")
                            currentIndex: page.typeIndex(type)
                            menu: ContextMenu {
                                MenuItem {
                                    text: I18n.t("type_text")
                                    onClicked: fieldsModel.setProperty(index, "type", "text")
                                }
                                MenuItem {
                                    text: I18n.t("type_secret")
                                    onClicked: fieldsModel.setProperty(index, "type", "secret")
                                }
                                MenuItem {
                                    text: I18n.t("type_url")
                                    onClicked: fieldsModel.setProperty(index, "type", "url")
                                }
                                MenuItem {
                                    text: I18n.t("type_email")
                                    onClicked: fieldsModel.setProperty(index, "type", "email")
                                }
                            }
                        }

                        IconButton {
                            id: removeButton
                            icon.source: "image://theme/icon-m-remove"
                            onClicked: fieldsModel.remove(index)
                        }
                    }

                    TextField {
                        width: parent.width
                        label: I18n.t("field_value")
                        text: value
                        echoMode: type === "secret" ? TextInput.Password : TextInput.Normal
                        onTextChanged: fieldsModel.setProperty(index, "value", text)
                    }
                }
            }

            Button {
                width: parent.width
                text: I18n.t("add_field")
                onClicked: fieldsModel.append({ label: "", type: "text", value: "" })
            }

            Button {
                width: parent.width
                text: I18n.t("save")
                onClicked: page.save()
            }

            ErrorLabel {
                code: vault.errorCode
                detail: vault.errorDetail
            }
        }
    }
}
