import QtQuick 2.0
import QtQuick.Controls 1.4

Item {
    property int btn_size: 24
    height: btn_size
    width:  (btn_size + 15) *3
    Row {
        layoutDirection: Qt.RightToLeft
        anchors.fill: parent
        anchors.right: parent.right
        SvgButton {
            source: "qrc:/assets/tools-icon.svg"
            width: btn_size
            height: parent.height
        }
        
        SvgButton {
            source: "qrc:/assets/info-icon.svg"
            width: btn_size
            height: parent.height
        }
        
        SvgButton {
            source: "qrc:/assets/update-icon.svg"
            width: btn_size
            height: parent.height
        }
        
        spacing: 15
    }
}
