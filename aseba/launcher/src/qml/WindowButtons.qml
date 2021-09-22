import QtQuick 2.0
import QtQuick.Controls 2.4

Item {
    anchors.topMargin: 30
    anchors.rightMargin: 30
    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        spacing: 15

        SvgButton {
            source: "qrc:/assets/launcher-icon-help.svg"
            width: 30
            height: 30
            onClicked: {
                Qt.openUrlExternally(qsTr("https://www.thymio.org/help/"))
            }
        }

        SvgButton {
            source: "qrc:/assets/launcher-icon-menu.svg"
            width: 30
            height: 30
            visible: true
            onClicked: {
                settingsMenu.visible = true
            }
        }
    }
}
