import QtQuick 2.12
import ".."

Rectangle {
    property int channel: 0
    property bool selected: false
    signal clicked()
    width: 40
    height: 40
    color: selected ? "#0a9eeb" : Style.dark
    radius: 180
    Text {
        anchors.centerIn: parent
        text: "%1".arg(channel + 1)
        font.pointSize: 15
        font.bold: true
        color: "white"
    }
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            parent.clicked()
        }
    }
}
