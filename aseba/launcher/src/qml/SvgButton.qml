import QtQuick 2.0

Item {
    id:button
    property url source
    signal clicked()
    Image {
        anchors.fill:  parent
        source: parent.source
        width: parent.width
        height: parent.height
        sourceSize: Qt.size(width, height)
    }
    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }
}
