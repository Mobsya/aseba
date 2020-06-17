import QtQuick 2.0
import QtQuick.Controls 2.3
import org.mobsya  1.0

Item {
    property var device: model.object
    property bool selectable: updateSelectable()
    property double progress

    Component.onCompleted: {
        device.groupChanged.connect(updateSelectable);
        device.statusChanged.connect(updateSelectable);

        device.firmwareUpdateProgress.connect(function (value) {
            progressBar.progress = value
        })

        launcher.selectedAppChanged.connect(updateSelectable);
    }


    function isThymio(device) {
        return device.type === ThymioNode.Thymio2
                || device.type === ThymioNode.Thymio2Wireless
                || device.type === ThymioNode.SimulatedThymio2
    }

    function updateSelectable() {
        selectable = (device.status === ThymioNode.Available || launcher.selectedApp.supportsWatchMode)
        &&  (!device.isInGroup || launcher.selectedApp.supportsGroups)
        &&  (launcher.selectedApp.supportsNonThymioDevices || isThymio(device) );
    }

    function upgradeFirmware() {


        if(device.type !== ThymioNode.Thymio2)
            return

        var component = Qt.createComponent("qrc:/qml/FirmwareUpdateDialog.qml");
        var dialog = component.createObject(launcher, {
                                            "oldVersion": device.fwVersion,
                                            "newVersion": device.fwVersionAvailable,
                                            "deviceName" : device.name
       });

       if (dialog === null) {
           console.log("Error creating dialog");
           return
       }
       dialog.yes.connect(function() {
           device.upgradeFirmware()
       })
       dialog.visible = true
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
            upgradeFirmware()
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
    property double preferredOpacity: {
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
            selection_view.launchSelectedAppWithSelectedDevice()
        }

        onPressAndHold: {
            if (mouse.source === Qt.MouseEventNotSynthesized)
                contextMenu.popup()
        }

        onHoveredChanged: {
            const host = device.endpoint.hostName
            if(host.endsWith(".local"))
                host = host.slice(0, -6)
            if(device.endpoint.isLocalhostPeer)
                tooltipText = qsTr("%1 on this computer").arg(device.name)
            else
                tooltipText = qsTr("%1 on %2").arg(device.name).arg(host)

            if(!selectable) {
                if(isInGroup && !launcher.selectedApp.supportsGroups) {
                    tooltipText += "\n" + qsTr("This device cannot be selected because it is in a group")
                }
                else if(!isThymio(device) && !launcher.selectedApp.supportsNonThymioDevices) {
                    tooltipText += "\n" + qsTr("This device is not compatible with %1").arg(launcher.selectedApp.name)
                }
                else {
                    tooltipText += "\n" + qsTr("This device cannot be selected because it is already being used")
                }
            }
        }
        ToolTip.text: tooltipText
        ToolTip.visible: device_mouse_area.containsMouse

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
                     opacity: preferredOpacity
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
                         onClicked: {
                             upgradeFirmware()
                         }
                         acceptedButtons: type === ThymioNode.Thymio2 ? MouseArea.LeftButton : null
                     }

                     ToolTip {
                         visible: update_icon_ma.containsMouse
                         text: (type === ThymioNode.Thymio2) ?
                                   qsTr("A new firmware is available!\nClick to install it") :
                                   qsTr("A new firmware is available!\nConnect the robot with a usb cable to install it")
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
                opacity: preferredOpacity
            }
            Item {
                width: parent.width
                height: 10
            }
            EditableDeviceNameInput {
                opacity: preferredOpacity
                id: textfield
                editable: capabilities & ThymioNode.Rename
                width: parent.width

                height: 48
                deviceName: name
                wrapMode: Text.WrapAnywhere

                
                onAccepted: {
                    
                    device.name = text
                    text = deviceName

                }

            }

            Item {
                id: progressBar
                property double progress: 0
                visible: progress > 0
                anchors.horizontalCenter: parent
                width: parent.width - 6
                anchors.left: parent.left
                anchors.margins: 3
                height: 3
                Rectangle {
                    id: progressBarBackground
                    anchors.left: parent.left
                    anchors.top : parent.top
                    height: parent.height
                    width: parent.width
                    color: Style.dark
                    radius: 2
                }
                Rectangle {
                    anchors.left: parent.left
                    anchors.top : parent.top
                    height: parent.height
                    width: parent.width * parent.progress
                    color: "#0a9eeb"
                    radius: 2
                }
            }
        }
    }
}
