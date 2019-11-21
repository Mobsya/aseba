import QtQuick.Controls 2.3

import QtQuick 2.11

Rectangle {
    color: "#535353"
    id:pane
    width: getBackPanelWidth()

    property ListModel entries: ListModel {}

    function getBackPanelWidth(){
        return 450
    }

    function thymio2PairingWizard(valiseMode) {
        var component = Qt.createComponent("qrc:/qml/wirelessconfigurator/WirelessWizardWarningDialog.qml");
        var dialog = component.createObject(launcher);

       if (dialog === null) {
           console.log("Error creating dialog");
           return
       }
       dialog.yes.connect(function() {
           var component = Qt.createComponent("qrc:/qml/wirelessconfigurator/WirelessConfigurator.qml");
           if (component.status === Component.Error) {
               console.log("Error loading WirelessConfigurator component:", component.errorString());
           }

           var dialog = component.createObject(launcher, {
                   "valiseMode": valiseMode
               }
           )
       })
       dialog.visible = true
    }


    function remoteConnectionDialog() {
        var component = Qt.createComponent("qrc:/qml/RemoteConnectionDialog.qml");
        var dialog = component.createObject(launcher);

       if (dialog === null) {
           console.log("Error creating dialog");
           return
       }
       dialog.visible = true
    }


    function onMenuEntryClicked(action) {
        pane.visible = false
        if(action === "playground") {
            Utils.launchPlayground()
        }
        else if(action === "playground-faq") {
            Qt.openUrlExternally(qsTr("https://www.thymio.org/thymio-simulator"))
        }
        else if(action === "thymio2-pairing") {
            thymio2PairingWizard(false)
        }
        else if(action === "thymio2-valise-pairing") {
            thymio2PairingWizard(true)
        }
        else if(action === "remote") {
            remoteConnectionDialog()
        }
    }

    function anchorToParent() {
        x =  launcher.width - (visible ? getBackPanelWidth() : 0)
    }

    Component.onCompleted: {
        parent.onWidthChanged.connect(anchorToParent)
        if(Utils.isPlaygroundAvailable) {
            entries.append( { "name": qsTr("Launch a Simulator"), action: "playground"})
            entries.append( { "name": qsTr("Download maps for the simulator"), action: "playground-faq"})
        }
        if(Utils.platformHasSerialPorts()) {
            entries.append( { "name": qsTr("Pair a Wireless Thymio to a Wireless dongle"), action: "thymio2-pairing"})
            entries.append( { "name": qsTr("Pair a case of Wireless Thymio"), action: "thymio2-valise-pairing"})
        }
        entries.append( { "name": qsTr("Connect to a remote host"), action: "remote"})
    }

    Item {
        id: settings_wrapper
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
            id: settings_list_view
            anchors.topMargin: 20
            orientation: ListView.Vertical
            anchors.top: top.bottom
            anchors.right: parent.right
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: getBackPanelWidth()
            model: entries
            delegate: Item {
                height: 30
                width: getBackPanelWidth()
                anchors.right:parent.right
                anchors.left:parent.left
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

            footer: Item {
                id: local_browser_switch
                visible: true
                height: 30
                width: getBackPanelWidth()
                anchors.right:parent.right
                anchors.left:parent.left
                anchors.bottom: parent.bottom
                anchors.leftMargin: 10
                CheckBox {
                    id: local_browser_checkbox
                    text: qsTr("Use your system default browser")
                    anchors.right: local_browser_switch.right
                    onClicked: {
                        Utils.setUseLocalBrowser(checked )
                    }
                    checked: Utils.getUseLocalBrowser()
                    indicator: Rectangle {
                        implicitWidth: 18
                        implicitHeight: 18
                        y: parent.height / 2 - height / 2
                        radius: 3
                        anchors.right: local_browser_checkbox_text.left
                        border.color: local_browser_checkbox.down ? "#276fa5" : "#389EEC"
                        Rectangle {
                            width: 10
                            height: 10
                            x: 4
                            y: 4
                            radius: 2
                            color: local_browser_checkbox.down ? "#276fa5" : "#389EEC"
                            visible: local_browser_checkbox.checked
                        }
                    }
                    contentItem: Text {
                        id: local_browser_checkbox_text
                        text: local_browser_checkbox.text
                        width: contentWidth
                        font: local_browser_checkbox.font
                        anchors.right: parent.right
                        opacity: enabled ? 1.0 : 0.3
                        color: local_browser_checkbox.down ? "#ededed" : "#fff"
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 10
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }
        }

        PropertyAnimation {
            id: showAnimation
            property: "x"
            target: pane
            from: launcher.width
            to: launcher.width - getBackPanelWidth()
            duration: 200
            easing.type: Easing.InOutQuad
        }
        onVisibleChanged: {
            x = launcher.width
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






        
