import QtQuick 2.0
import QtQuick.Layouts 1.12
import org.mobsya  1.0
import ".."

Rectangle {
    id: wirelessConfigurator
    color: Style.light
    anchors.fill: parent
    TitleBar {}

    property bool valiseMode: false
    property bool advancedMode: false
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


    property var pairedDongleUUIDs: []
    property var pairedDongleNetworkIds: []
    property var pairedRobotUUIDs: []

    property var robotCount: 0


    function getRandomInt(min, max) {
      min = Math.ceil(min);
      max = Math.floor(max);
      return Math.floor(Math.random() * (max - min + 1)) + min;
    }

    function getRandomChannel() {
        return getRandomInt(0, 2)
    }

    function getChannel() {
        return valiseMode ? robotCount % 3 : getRandomChannel()
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

        if(!dongles) {
            errorBanner.text = qsTr("Unable to list the dongles - please relaunch Thymio Suite")
            return
        }

        const nodes = usbThymios()

        if(waitingForUnplug) {
            if(dongles.length === 0 || pairedDongleUUIDs.indexOf(dongles[0])  < 0 ) {
                errorBanner.text = qsTr("Pairing successful")
                nextButton.visible = true
                pairButton.visible = false
                waitingForUnplug = false
            }
            return
        }

        if(advancedMode && dongles.length === 1) {
            let req = donglesManager.dongleInfo(dongles[0])
            Request.onFinished(req, function(status, res) {
                console.log("Info for dongle %1 : network %2 - channel : %3"
                    .arg(dongles[0])
                    .arg(res.networkId())
                    .arg(res.channel()))

                selectedNetworkId = getNonDefaultNetworkId(res.networkId())
                selectedChannel = getChannel()
            })
        }

        //Dongles have a new UUID each time they are unplugged
        //If we see a dongle we previously paired, ask the user to uplug it
        if(dongles.length === 1 && pairedDongleUUIDs.indexOf(dongles[0]) >= 0) {
            errorBanner.text = qsTr("Please unplug the dongle to complete the process")
            nextButton.visible = false
            pairButton.visible = false
            waitingForUnplug = true
            return
        }

        if(nextButton.visible)
            return;

        errorBanner.text = ""
        if (dongles.length < 1) {
            errorBanner.text = qsTr("Please plug a wireless dongle in a USB port of this computer")
            return
        }
        if (dongles.length > 1) {
            errorBanner.text = qsTr("%1 dongles detected - Please plug a single dongle while pairing a Thymio")
                .arg(dongles.length)
            return
        }



        if(nodes.length < 1) {
            errorBanner.text = qsTr("Please plug a Thymio 2 Wireless Robot in a USB port of this computer")
            return
        }
        if (nodes.length > 1) {
            errorBanner.text = qsTr("%1 Thymios detected - Please plug a single Thymio 2 Wireless Robot to pair it")
                .arg(nodes.length)
            return
        }

        if(valiseMode && nodes.length === 1 && pairedRobotUUIDs.indexOf(nodes[0].id.toString()) >= 0) {
            errorBanner.text = qsTr("You have alredy paired this robot")
            return
        }


        selectedRobotId = nodes[0].id
        selectedDongleId = dongles[0]
        ready = true
    }

    function pairRobotAndDongle(robotId, dongleId, networkId, channel) {
        console.log("Pairing node %1 to dongle %2; network %3 - channel %4"
            .arg(robotId.toString())
            .arg(dongleId)
            .arg(networkId)
            .arg(channel))

        ready = false
        const request = donglesManager.pairThymio2Wireless(dongleId, robotId, networkId, channel)

        pairedDongleUUIDs.push(dongleId)
        pairedRobotUUIDs.push(robotId.toString())
        pairedDongleNetworkIds.push(networkId)


        Request.onFinished(request, function(status, res) {
            console.log("Pairing complete: network %1 - channel %2"
                .arg(res.networkId())
                .arg(res.channel()))
            robotCount ++
            waitingForUnplug = true
            updateState()
        })
    }

    function pairSelectedRobotAndDongle() {
        if(advancedMode) {
            if(!ready)
                return
            selectedNetworkId = parseInt(networkSelector.text, 16)
            selectedChannel = channelSelector.channel
            pairRobotAndDongle(selectedRobotId, selectedDongleId, selectedNetworkId, selectedChannel)
            return
        }

        updateState()
        if(!ready)
            return
        Request.onFinished(req, function(status, res) {
            console.log("Info for dongle %1 : network %2 - channel : %3"
                .arg(selectedDongleId)
                .arg(res.networkId())
                .arg(res.channel()))

            selectedNetworkId = getNonDefaultNetworkId(res.networkId())
            selectedChannel = getChannel()

            if(valiseMode && pairedDongleNetworkIds.indexOf(selectedNetworkId) >= 0) {
                ready = false
                errorBanner.text = qsTr("You have alredy paired this dongle with another robot")
                return
            }
            pairRobotAndDongle(selectedRobotId, selectedDongleId, selectedNetworkId, selectedChannel)
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
        id:  simpleModeLayout
        visible: !advancedMode


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
                text: {
                    let txt = qsTr("<ol><li><b>Plug the Thymio you want to pair using the USB cable</b></li><br/><li><b>Plug the USB dongle</b></li></ol>")
                    if (valiseMode) {
                        txt += qsTr("<p><em>Tip: Put a sticker dot of the same color on both the thymio and the dongle to identify them!</em></p>")
                    }
                    return txt
                }
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
        id:  advancedModeLayout
        visible: advancedMode
        anchors.fill: parent
        anchors.topMargin: 30 + 60
        onVisibleChanged: {
            if(visible) {
                updateState()
            }
        }

        Item {
            width : parent.width
            anchors.top: parent.top
            anchors.bottom: advancedModeOuterFrame.top
            anchors.margins: 30

            Row {
                spacing: 60
                anchors.fill: parent
                Item {
                    width: (parent.width / 2) - 30
                    height: parent.height
                    Column {
                        anchors.fill: parent
                        spacing: 30
                        Image {
                             source : "qrc:/assets/one-thymio-one-dongle.svg"
                             fillMode: Image.PreserveAspectFit
                             height: 120
                             width: 120
                             smooth: true
                             antialiasing: true
                             anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                             id:txtOneDongle
                             color: "white"
                             font.pointSize: 10
                             anchors.horizontalCenter: parent.horizontalCenter
                             horizontalAlignment:Text.AlignHCenter
                             wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                             text: {
                                 let txt = qsTr("<p><b>To pair one robot to one dongle on an independant network:</b><br/></p>")
                                 txt +=  qsTr("<p>Use different <b>Channels</b> and different <b>network identifiers</b> for each robot/dongle pair you have</p>")
                                 return txt
                             }
                             width: parent.width
                        }
                    }
                }
                Item {
                    width: (parent.width / 2) - 30
                    height: parent.height
                    Column {
                        anchors.fill: parent
                        spacing: 30
                        Image {
                             source : "qrc:/assets/multiple-thymio-one-dongle.svg"
                             fillMode: Image.PreserveAspectFit
                             height: 120
                             width: 120
                             smooth: true
                             antialiasing: true
                             anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                             color: "white"
                             font.pointSize: 10
                             anchors.horizontalCenter: parent.horizontalCenter
                             horizontalAlignment:Text.AlignHCenter
                             wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                             text: {
                                 let txt = qsTr("<p><b>To pair multiple Thymios to one dongle:</b><br/></p>")
                                 txt +=  qsTr("<p>Use the same <b>Channels</b> and <b>network identifier</b> for every robot you want in the same network</p>")
                                 return txt
                             }
                             width: parent.width
                        }
                    }
                }
            }
        }


        Rectangle{
            id:advancedModeOuterFrame
            color: Style.mid
            height: 290
            width: parent.width
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 80
            Item {
                anchors.fill: parent
                anchors.topMargin: 80
                RowLayout {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 130
                    Item {
                        width: 230
                        height: 130
                        Text {
                            text: qsTr("01. Select a channel")
                            color: "white"
                            font.bold: true
                            font.pointSize: 10
                        }
                        ChannelSelector {
                            id: channelSelector
                            width: parent.width
                            height: 40
                            anchors.bottom: parent.bottom
                            channel: selectedChannel ? selectedChannel : -1
                        }
                    }
                    Item {
                        width: 230
                        height: 130
                        Text {
                            text: qsTr("<b>02. Define the network identifier</b><br/>4 characters in hexadecimal (0 to 9 and A-F)")
                            color: "white"
                            font.pointSize: 10
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                            width: parent.width
                        }
                        NetworkIdInput {
                            id: networkSelector
                            width: parent.width
                            height: 40
                            anchors.bottom: parent.bottom
                            text: selectedNetworkId
                        }
                    }
                    Item {
                        width: 230
                        height: 130
                        Text {
                            text: qsTr("<b>03. Click on start pairing</b>")
                            color: "white"
                            font.pointSize: 10
                            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                            width: parent.width
                        }
                        Button {
                            id : pairButtonAdvanced
                            text: qsTr("Pair !")
                            enabled: ready
                            onClicked: {
                                pairSelectedRobotAndDongle()
                            }
                            anchors.bottom: parent.bottom
                        }
                    }
                }
            }
        }
    }

    Item {
        visible: !advancedMode
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 60
        height: 40
        width: parent.width
        Row {
            anchors.centerIn: parent
            spacing: 22
            Button {
                id : pairButton
                text: valiseMode ? qsTr("Pair robot #%1").arg(robotCount+1) : qsTr("Pair !")
                enabled: ready
                onClicked: {
                    pairSelectedRobotAndDongle()
                }
            }
            Button {
                visible: false
                id : nextButton
                text: valiseMode ? qsTr("Pair the next Robot") : qsTr("Pair another robot")
                onClicked: {
                    pairButton.visible = true
                    nextButton.visible = false
                    updateState()
                }
            }

            Button {
                id : advancedButton
                visible: !valiseMode
                text: advancedMode ? qsTr("Simple Mode") : qsTr("Advanced Mode")
                onClicked: {
                    advancedMode = ! advancedMode
                }
            }
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

