import QtQuick 2.0
import ".."

Rectangle  {
    anchors.top: parent.top
    width: parent.width
    height: Style.titlebar_height
    color: Style.dark
    Text {
        text: "Wireless Configurator"
        color: "white"
        font.bold: true
        font.family: "Roboto"
        font.pointSize: 18
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 22
        anchors.left: parent.left
        smooth: true
        antialiasing: true
    }

    SvgButton {
        source: "qrc:/assets/launcher-icon-close.svg"
        height: 22
        width : 22
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: (parent.height - height) / 2
        onClicked: {
            wirelessConfigurator.destroy()
        }
    }
}