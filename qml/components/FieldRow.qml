import QtQuick 2.2
import Sailfish.Silica 1.0

Column {
    id: field

    property string caption
    property string value
    property bool secret: false
    property bool copyable: true
    property string actionIcon
    property bool actionVisible: true

    signal actionClicked()

    width: parent ? parent.width : 0
    spacing: Theme.paddingSmall

    Label {
        text: field.caption
        color: Theme.secondaryColor
        font.pixelSize: Theme.fontSizeSmall
        width: parent.width
    }

    Row {
        width: parent.width
        spacing: Theme.paddingSmall

        Label {
            id: valueLabel
            width: parent.width - (actionButton.visible ? actionButton.width : 0)
                   - (copyButton.visible ? copyButton.width : 0)
                   - (revealButton.visible ? revealButton.width : 0)
            text: field.secret && !revealed ? "\u2022\u2022\u2022\u2022\u2022\u2022\u2022\u2022" : field.value
            color: Theme.primaryColor
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeMedium
        }

        IconButton {
            id: actionButton
            visible: field.actionIcon.length > 0 && field.actionVisible
            icon.source: field.actionIcon
            onClicked: field.actionClicked()
        }

        IconButton {
            id: revealButton
            visible: field.secret
            icon.source: revealed ? "image://theme/icon-splus-hide-password" : "image://theme/icon-splus-show-password"
            onClicked: revealed = !revealed
        }

        IconButton {
            id: copyButton
            visible: field.copyable && field.value.length > 0
            icon.source: "image://theme/icon-m-clipboard"
            onClicked: vault.copySecret(field.value)
        }
    }

    property bool revealed: false
}
