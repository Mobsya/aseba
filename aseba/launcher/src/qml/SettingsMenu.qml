import QtQuick 2.11
import QtQuick.Controls 2.4


Rectangle {
    property bool showExperimentalApps: showExperimentalAppsCheckbox.checked

    color: "#535353"
    id:pane
    width: 350
    ListModel {
        id: entries
        ListElement {
            name: qsTr("Launch a Simulator")
            action: "playground"
        }
    }
    function onMenuEntryClicked(action) {
        if(action === "playground") {
            Utils.launchPlayground()
        }
    }

    Item {
        anchors.fill: parent
        anchors.margins: 10
        Item {
            id:top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 20

            SvgButton {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                source: "qrc:/assets/launcher-icon-close.svg"
                height: 16
                width: height
                onClicked: {
                    pane.visible = false
                }
            }

            Text {
                text: qsTr("Tools")
                color: "white"
                font.bold: true
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        ListView {
            anchors.topMargin: 20
            orientation: ListView.Vertical
            anchors.top: top.bottom
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 350
            model: entries
            delegate: Item {
                height: 30
                width: 350
                Text {
                    anchors.right: parent.right
                    width: contentWidth
                    color: ma.containsMouse? "#009FE3" : "white"
                    text: name
                }
                MouseArea {
                    anchors.fill: parent
                    id:ma
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        pane.onMenuEntryClicked(action)
                    }
                }
            }
        }

        CheckBox {
            id: showExperimentalAppsCheckbox
            contentItem: Text {
                leftPadding: parent.indicator.width + parent.spacing
                text: parent.text
                font: parent.font
                color: "white"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
           }
           text: qsTr("Show Experimental Applications")
           checked: false
           anchors.bottom: parent.bottom
           palette.text:"#0a9eeb"
       }

        PropertyAnimation {
            id: showAnimation
            property: "x"
            target: pane
            from: pane.parent.width
            to: pane.parent.width - 350
            duration: 200
            easing.type: Easing.InOutQuad
        }
        onVisibleChanged: {
            x = pane.parent.width
            showAnimation.running = true
        }
    }
    InverseMouseArea {
        enabled: pane.visible
        onPressed:  {
            pane.visible = false
        }
        anchors.fill: parent
    }
}
