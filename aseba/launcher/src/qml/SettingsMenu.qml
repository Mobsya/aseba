import QtQuick 2.11

Rectangle {
    color: "#535353"
    id:pane
    width: 350

    property ListModel entries: ListModel {}

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
        x =  launcher.width - (visible ? 350 : 0)
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

        PropertyAnimation {
            id: showAnimation
            property: "x"
            target: pane
            from: launcher.width
            to: launcher.width - 350
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
