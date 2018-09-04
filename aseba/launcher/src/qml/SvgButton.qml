import QtQuick 2.0
import QtGraphicalEffects 1.0

Item {
    id:button
    property url source
    signal clicked()
    Image {
        id:image
        anchors.fill:  parent
        source: parent.source
        width: parent.width
        height: parent.height
        sourceSize: Qt.size(width, height)
        antialiasing: true
    }
    MouseArea {
        id: mouse_area
        anchors.fill: parent
        onClicked: parent.clicked()
        hoverEnabled: true
    }

    BrightnessContrast {
       anchors.fill: image
       source: image
       brightness: mouse_area.containsMouse ? 0.5 : 0
       contrast: 0
   }
}
