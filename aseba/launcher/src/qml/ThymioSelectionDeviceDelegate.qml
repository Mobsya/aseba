import QtQuick 2.0
import QtQuick.Controls 2.3
import org.mobsya  1.0

Item {
    property var device: model.object

    Action {
        text: "Rename"
        id: renameAction
        onTriggered: if(textfield.editable) textfield.readOnly = false
    }
    Action {
        text: "Upgrade Firmware"
        id: upgradeAction
    }
    Action {
        text: "Reset"
        id: resetAction
    }
    Action {
        text: "Stop"
        id: stopAction
        onTriggered: device.stop()
    }

    id:item
    height: 172
    width : 172
    property bool selected: device_view.currentIndex === index
    opacity: {
        switch(status) {
        case ThymioNode.Ready:
        case ThymioNode.Available:
                return selected ? 1 : 0.8;
        default: return 0.5
        }
    }

    property bool device_ready: status === ThymioNode.Ready || status === ThymioNode.Available

    MouseArea {
        id: device_mouse_area
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            if (mouse.button === Qt.RightButton)
                contextMenu.popup(item)
            else
                device_view.currentIndex = index
                device_view.selectedDevice = device
        }
        onPressAndHold: {
            if (mouse.source === Qt.MouseEventNotSynthesized)
                contextMenu.popup()
        }

        cursorShape: device_ready ? Qt.PointingHandCursor : null

        DeviceContextMenu {
            id: contextMenu
            device: device
            onOpen: {
                if(!menu)
                    return
                if(capabilities & ThymioNode.Rename) {
                    menu.addAction(renameAction)
                }
                if(capabilities & ThymioNode.ForceResetAndStop) {
                    menu.addAction(stopAction)
                }
                //menu.addAction(upgradeAction)
            }
        }

    }

    Rectangle {
        id: selectable_area
        color: device_mouse_area.containsMouse ? Style.light : "transparent"
        border.width: item.selected ? 1.3333 : 0
        border.color: "#0a9eeb"
        anchors.fill: parent
        anchors.horizontalCenter: parent.horizontalCenter

        Column {
            width : 142
            height: 120
            anchors.centerIn: parent

            Item {
                width :  90
                anchors.horizontalCenter: parent.horizontalCenter
                height: 12


                BatteryIndicator {
                    anchors.verticalCenter: parent.verticalCenter
                    height: 8
                    anchors.left: parent.left
                    anchors.leftMargin: 2

                }

                Image {
                     id: icon;
                     source : "qrc:/assets/update-icon.svg"
                     fillMode: Image.PreserveAspectFit
                     anchors.verticalCenter: parent.verticalCenter
                     height: 12
                     anchors.right: parent.right
                     anchors.leftMargin: 2
                     smooth: true
                     antialiasing: true
                }
            }

            Item {
                width: parent.width
                height: 8
            }

            Image {
                source: {
                    switch (type) {
                     case ThymioNode.Thymio2:
                     case ThymioNode.Thymio2Wireless:
                        return "qrc:/assets/thymio.svg"
                     case ThymioNode.SimulatedThymio2:
                        return "qrc:/assets/simulated_thymio.svg"
                     default:
                        return "qrc:/assets/dummy_node.svg"
                    }
                }
                width :  90
                fillMode:Image.PreserveAspectFit
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Item {
                width: parent.width
                height: 10
            }
            EditableDeviceNameInput {
                id: textfield
                editable: capabilities & ThymioNode.Rename
                width: parent.width
                height: 40
                deviceName: name
                onAccepted: {
                    device.name = text
                    text = deviceName
                }
            }
        }
    }
}