import QtQuick 2.7
import Thymio 1.0

Item {
    property AsebaClient aseba
    property int selected
    signal itemSelected

    Rectangle {
        color: "#3b3b3b"
        anchors.fill: parent
    }

    Text {
        text: qsTr("No Thymio connected :(")
        anchors.centerIn: parent
        color: "#FFFFFF"
        font.pointSize: 20
        visible: view.count == 0
    }

    GridView {
        id:view
        anchors.fill: parent
        topMargin: 20
        leftMargin: 20
        anchors.leftMargin: 10
        model: aseba.model
        cellWidth:180
        delegate: Item {
            height: content.height
            width: content.width
            Column {
                id: content
                Image {
                    id:image
                    source: "images/icon-thymio.svg"
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 100
                    fillMode: Image.PreserveAspectFit

                }
                Text {
                    id:label
                    text: name;
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "#FFFFFF"
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    thymioselectionpane.selected = nodeId
                    itemSelected()
                }
            }
        }
    }
}
