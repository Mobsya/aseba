import QtQuick 2.12

Rectangle {
    property alias text: text1.text
    property alias enabled: mouse_area.enabled
    signal clicked
    height: 40
    width : 220
    radius: 20
    color: mouse_area.containsMouse ? "#57c6ff" : "#0a9eeb"
    opacity: enabled ? 1 : 0.3
    Text {
        id: text1
        font.family: "Roboto"
        font.pointSize: 12
        color : "white"
        anchors.centerIn: parent

    }
    MouseArea {
        anchors.fill: parent
        hoverEnabled: enabled
        id: mouse_area
        cursorShape: parent.enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
    }

    Component.onCompleted: {
        mouse_area.clicked.connect(clicked)
    }
}
