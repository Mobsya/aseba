import QtQuick 2.0
import org.mobsya  1.0
import ".."

Rectangle {
    id: wirelessConfigurator
    color: Style.light
    anchors.fill: parent
    TitleBar {}

    property var localEndpoint: client.localEndpoint
    property var donglesManager: null
    property var dongles: null
    property var nodes: null
    property var ready: false
    property var waitingForUnplug: false

    property var selectedDongleId: null
    property var selectedRobotId: null
    property var selectedNetworkId: null
    property var selectedChannel: null


    property var pairedDongleUUID: []


    function getRandomInt(min, max) {
      min = Math.ceil(min);
      max = Math.floor(max);
      return Math.floor(Math.random() * (max - min + 1)) + min;
    }

    function getRandomChannel() {
        return getRandomInt(0, 2)
    }

    function getRandomNetworkId() {
        return getRandomInt(0, 0xFFFF)
    }

    function getNonDefaultNetworkId(networkId) {
        if (networkId === 0x404f)
            return getRandomNetworkId()
        return networkId
    }

    function usbThymios() {
        if(!nodes)
            return []
        let arr = []
        let i;
        for (i = 0; i < nodes.length; i++) {
            if (nodes[i].type === ThymioNode.Thymio2)
                arr.push(nodes[i])
        }
        return arr;
    }

    function updateState() {
        ready = false

        selectedRobotId = null
        selectedDongleId = null
        selectedNetworkId = null
        selectedChannel = null

        if(!dongles)
            return

        const nodes = usbThymios()

        if(waitingForUnplug) {
            if(dongles.length === 0 && nodes.length === 0) {
                errorBanner.text = qsTr("Pairing successful")
                waitingForUnplug = false
            }
            return
        }

        //Dongles have a new UUID each time they are unplugged
        //If we see a dongle we previously paired, ask the user to uplug it
        if(dongles.length >= 0 && pairedDongleUUID.indexOf(dongles[0]) >= 0) {
            errorBanner.text = qsTr("Please unplug the dongle to complete the process")
            waitingForUnplug = true
            return
        }

        errorBanner.text = ""
        if (dongles.length < 1) {
            errorBanner.text = qsTr("Please plug a wireless dongle in an USB port of this computer")
            return
        }
        if (dongles.length > 1) {
            errorBanner.text = qsTr("%1 dongles detected - Please plug a single dongle while pairing a Thymio")
                .arg(dongles.length)
            return
        }



        if(nodes.length < 1) {
            errorBanner.text = qsTr("Please plug a Thymio 2 Wireless Robot in an USB port of this computer")
            return
        }
        if (nodes.length > 1) {
            errorBanner.text = qsTr("%1 Thymios detected - Please plug a single Thymio 2 Wireless Robot to pair it")
                .arg(nodes.length)
            return
        }

        selectedRobotId = nodes[0].id
        selectedDongleId = dongles[0]
        ready = true
    }

    function pairSelectedRobotAndDongle() {
        updateState()
        if(!ready)
            return

        console.log("Requesting info for dongle %1"
            .arg(selectedDongleId))
        let req = donglesManager.dongleInfo(dongles[0])
        Request.onFinished(req, function(status, res) {
            console.log("Info for dongle %1 : network %2 - channel : %3"
                .arg(selectedDongleId)
                .arg(res.networkId())
                .arg(res.channel()))

            selectedNetworkId = getNonDefaultNetworkId(res.networkId())
            selectedChannel = getRandomChannel()
            console.log("Pairing node %1 to dongle %2; network %3 - channel %4"
                .arg(selectedRobotId.toString())
                .arg(selectedDongleId)
                .arg(selectedNetworkId)
                .arg(selectedChannel))

            pairedDongleUUID.push(selectedDongleId)

            const request = donglesManager.pairThymio2Wireless(selectedDongleId,
                                                              selectedRobotId, selectedNetworkId, selectedChannel)
            Request.onFinished(request, function(status, res) {
                console.log("Pairing complete: network %1 - channel %2"
                    .arg(res.networkId())
                    .arg(res.channel()))
                waitingForUnplug = true
                updateState()
            })
        })


    }


    Component.onCompleted: {
        client.localEndpointChanged.connect(function () {
            localEndpoint = client.localEndpoint
            donglesManager = localEndpoint.donglesManager
            dongles = donglesManager.dongles
            donglesManager.donglesChanged.connect(function () {
                dongles = null
                if(donglesManager) {
                    dongles = donglesManager.dongles
                    console.log("Dongles connected: %1".arg(dongles.length))
                }
                updateState()
            })
            localEndpoint.nodesChanged.connect(function () {
                nodes = null
                if(localEndpoint)
                    nodes = localEndpoint.nodes
                updateState()
            })
        })
        localEndpoint = client.localEndpoint
        if(localEndpoint) {
            donglesManager = localEndpoint.donglesManager
            dongles = donglesManager.dongles
            nodes = localEndpoint.nodes
            updateState()
        }
    }

    Item {
        anchors.fill: parent
        anchors.margins: 60

        Item {
            height: parent.height
            width : parent.width / 2
            anchors.top: parent.top
            anchors.left: parent.left

            Image {
                 source : "qrc:/assets/usb-cable-and-dongle.svg"
                 fillMode: Image.PreserveAspectFit
                 anchors.centerIn: parent
                 smooth: true
                 antialiasing: true
            }
        }

        Item {
            height: parent.height
            width : parent.width / 2
            anchors.top: parent.top
            anchors.right: parent.right

            Text {
                text: qsTr("<ol><li><b>Plug the Thymio you want to pair using the USB cable</b></li><br/><li><b>Plug the USB dongle</b></li></ol>")
                font.pointSize: 15
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.right: parent.right
                wrapMode: Text.WordWrap
                color: "white"
                clip: true
            }
        }
    }


    Item {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 60
        height: 40
        width: parent.width
        Row {
            anchors.centerIn: parent
            spacing: 22
            Button {
                text: qsTr("Start Pairing")
                enabled: ready
                onClicked: {
                    pairSelectedRobotAndDongle()
                }
            }
            /*Button {
                text: qsTr("Advanced Mode")
            }*/
        }
    }

    Rectangle {
        property alias text: message.text
        visible: message.text != ""
        id: errorBanner
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        height: 40
        color: "#0a9eeb"
        Row {
            anchors.centerIn: parent
            spacing: 22
            Image {
                source : "qrc:/assets/alert.svg"
                fillMode: Image.PreserveAspectFit
                anchors.verticalCenter: parent.verticalCenter
                smooth: true
                height: 22
                antialiasing: true
            }
            Text {
                text: ""
                id: message
                font.family: "Roboto Bold"
                font.pointSize: 12
                color : "white"
            }
        }
    }
}

