import QtQuick 2.0
import QtQuick.Controls 2.3
import org.mobsya  1.0

Item {
    property var device: model.object
    property bool selectable: updateSelectable()

     Component.onCompleted: {
        device.groupChanged.connect(updateSelectable);
        device.statusChanged.connect(updateSelectable);
        launcher.selectedAppChanged.connect(updateSelectable);
     }

    function updateSelectable() {
        selectable = (device.status === ThymioNode.Available || launcher.selectedApp.supportsWatchMode)
        &&  (!device.isInGroup || launcher.selectedApp.supportsGroups);
    }

    Action {
        text: "Rename"
        id: renameAction
        onTriggered: if(textfield.editable) textfield.readOnly = false
    }
    Action {
        text: "Upgrade Firmware"
        id: upgradeAction
        onTriggered: {
            device.upgradeFirmware()
        }
    }
    Action {
        text: "Reset"
        id: resetAction
    }
    Action {
        text: "Stop"
        id: stopAction
        onTriggered: {
            var result = device.stop()
            Request.onFinished(result, function(status, res) {
                console.log(res)
            })
        }
    }

    id:item
    height: 172
    width : 172
    property bool selected: device_view.selectedDevice === device
    opacity: {
        if(!selectable)
            return 0.3
        switch(status) {
        case ThymioNode.Ready:
        case ThymioNode.Available:
                return selected ? 1 : 0.8;
        default: return 0.5
        }
    }

    property bool device_ready: status === ThymioNode.Ready || status === ThymioNode.Available
    property string tooltipText: ""

    MouseArea {

        id: device_mouse_area
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            device_view.currentIndex = -1
            device_view.selectedDevice = undefined

            if (mouse.button === Qt.RightButton)
                contextMenu.popup(item)
            else if(selectable) {
                device_view.currentIndex = index
                device_view.selectedDevice = device
            }
        }
        onDoubleClicked: {
            if(!selectable)
                return
            const selectedAppLauncher = launcher.selectedAppLauncher;
            if(!selectedAppLauncher) {
                console.error("No launch function")
            }
            else if(!selectedAppLauncher(device)) {
                console.error("could not launch app with device %2".arg(device))
            }
            updateSelectable()
        }

        onPressAndHold: {
            if (mouse.source === Qt.MouseEventNotSynthesized)
                contextMenu.popup()
        }

        onHoveredChanged: {
            tooltipText = ""
            if(!selectable) {
                if(isInGroup && !launcher.selectedApp.supportsGroups) {
                    tooltipText = qsTr("This device cannot be selected because it is in a group")
                }
                else {
                    tooltipText = qsTr("This device cannot be selected because it is already being used")
                }
            }
        }
        ToolTip.text: tooltipText
        ToolTip.visible: tooltipText != "" && device_mouse_area.containsMouse

        cursorShape: device_ready ? Qt.PointingHandCursor : null

        DeviceContextMenu {
            id: contextMenu
            onOpen: {
                if(!menu)
                    return

                menu.device = device
                if(capabilities & ThymioNode.Rename) {
                    menu.addAction(renameAction)
                }
                if(capabilities & ThymioNode.ForceResetAndStop) {
                    menu.addAction(stopAction)
                }
                if((capabilities & ThymioNode.FirmwareUpgrade) && device.hasAvailableFirmwareUpdate) {
                    menu.addAction(upgradeAction)
                }
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


                /*BatteryIndicator {
                    anchors.verticalCenter: parent.verticalCenter
                    height: 8
                    anchors.left: parent.left
                    anchors.leftMargin: 2

                }*/

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
                     visible: device.hasAvailableFirmwareUpdate
                     MouseArea {
                         id:update_icon_ma
                         anchors.fill: parent
                         hoverEnabled: true
                     }

                     ToolTip {
                         visible: update_icon_ma.containsMouse
                         text: qsTr("A new firmware is available!")
                     }
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