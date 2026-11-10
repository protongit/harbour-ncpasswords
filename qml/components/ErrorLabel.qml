import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"

Label {
    id: label

    property string code
    property string detail

    visible: code.length > 0
    text: {
        if (!code) {
            return ""
        }
        var key = "error_" + code
        var translated = I18n.t(key)
        if (translated === key) {
            translated = I18n.t("error_generic") + " (" + code + ")"
        }
        if (detail.length > 0) {
            translated += "\n" + detail
        }
        return translated
    }
    width: parent ? parent.width : 0
    color: Theme.highlightColor
    font.pixelSize: Theme.fontSizeSmall
    wrapMode: Text.Wrap
    horizontalAlignment: Text.AlignHCenter
}
