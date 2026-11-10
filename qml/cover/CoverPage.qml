import QtQuick 2.2
import Sailfish.Silica 1.0
import "../js"

CoverBackground {
    id: cover

    property var totp: vault.currentTotp

    Component.onCompleted: vault.logDebug("CoverPage loaded")

    Column {
        anchors.fill: parent
        anchors.margins: Theme.paddingSmall
        spacing: Theme.paddingSmall

        Label {
            id: title
            width: parent.width
            text: I18n.t("app_name")
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }

        Row {
            width: parent.width
            spacing: Theme.paddingSmall

            Item {
                id: leftArrow
                width: Theme.itemSizeSmall
                height: width
                visible: vault.state === "unlocked" && vault.totpCount > 1

                Icon {
                    anchors.centerIn: parent
                    source: "image://theme/icon-splus-left"
                    color: Theme.highlightColor
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: vault.previousTotp()
                }
            }

            Column {
                width: parent.width - (leftArrow.visible ? leftArrow.width : 0)
                       - (rightArrow.visible ? rightArrow.width : 0)
                       - (leftArrow.visible || rightArrow.visible ? Theme.paddingSmall * 2 : 0)
                spacing: Theme.paddingSmall / 2

                Label {
                    width: parent.width
                    text: cover.totp.issuer !== undefined && cover.totp.issuer.length > 0
                          ? cover.totp.issuer
                          : (cover.totp.label !== undefined ? cover.totp.label : "")
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }

                Label {
                    width: parent.width
                    text: cover.totp.formattedCode !== undefined
                          && cover.totp.formattedCode.length > 0
                          ? cover.totp.formattedCode
                          : "------"
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeLarge
                    horizontalAlignment: Text.AlignHCenter
                }

                Rectangle {
                    width: parent.width
                    height: 3
                    color: "#33ffffff"

                    Rectangle {
                        height: parent.height
                        width: parent.width
                               * ((cover.totp.remaining !== undefined ? cover.totp.remaining : 0)
                                  / (cover.totp.period !== undefined && cover.totp.period > 0
                                     ? cover.totp.period : 30))
                        color: Theme.highlightColor
                    }
                }

                Label {
                    width: parent.width
                    text: cover.totp.remaining !== undefined
                          ? cover.totp.remaining + "s"
                          : ""
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Item {
                id: rightArrow
                width: Theme.itemSizeSmall
                height: width
                visible: vault.state === "unlocked" && vault.totpCount > 1

                Icon {
                    anchors.centerIn: parent
                    source: "image://theme/icon-splus-right"
                    color: Theme.highlightColor
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: vault.nextTotp()
                }
            }
        }

        Label {
            width: parent.width
            text: vault.state === "unlocked" && vault.totpCount > 0
                  ? (vault.totpIndex + 1) + "/" + vault.totpCount
                  : (vault.state === "unlocked" ? I18n.t("no_totp") : I18n.t("locked"))
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
