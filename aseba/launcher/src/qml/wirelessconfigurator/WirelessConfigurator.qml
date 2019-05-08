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

    property var selectedDongleId: null
    property var selectedRobotId: null
    property var selectedNetworkId: null
    property var selectedChannel: null


    function getRandomInt(min, max) {
      min = Math.ceil(min);
      max = Math.floor(max);
      return Math.floor(Math.random() * (max - min + 1)) + min;
    }

    function getRandomChannel() {
        return getRandomInt(1, 3)
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
        const nodes = usbThymios()
        if(nodes.length < 1) {
            errorBanner.text = qsTr("Please plug a Thymio 2 Wireless Robot in an USB port of this computer")
            return
        }
        if (nodes.length > 1) {
            errorBanner.text = qsTr("%1 Thymios detected - Please plug a single Thymio 2 Wireless Robot to pair it")
                .arg(nodes.length)
            return
        }
        ready = true

        selectedRobotId = nodes[0].id
        selectedDongleId = dongles[0]
        selectedNetworkId = getNonDefaultNetworkId(donglesManager.networkId(dongles[0]))
        selectedChannel = getRandomChannel()
    }

    function pairSelectedRobotAndDongle() {
        updateState()
        if(!ready)
            return
        const request = localEndpoint.pairThymio2Wireless(selectedDongleId, selectedRobotId, selectedNetworkId, selectedChannel)
    }


    Component.onCompleted: {
        client.localEndpointChanged.connect(function () {
            localEndpoint = client.localEndpoint
            donglesManager = localEndpoint.donglesManager
            dongles = donglesManager.dongles
            donglesManager.donglesChanged.connect(function () {
                dongles = donglesManager.dongles
                console.log("Dongles connected: %1".arg(dongles.length))
                updateState()
            })
            localEndpoint.nodesChanged.connect(function () {
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

