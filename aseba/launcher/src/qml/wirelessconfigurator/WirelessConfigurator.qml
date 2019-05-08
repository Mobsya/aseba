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

    Component.onCompleted: {
        client.localEndpointChanged.connect(function () {
            localEndpoint = client.localEndpoint
            donglesManager = localEndpoint.donglesManager
            dongles = donglesManager.dongles
            donglesManager.donglesChanged.connect(function () {
                dongles = donglesManager.dongles
                console.log("Dongles connected: %1".arg(dongles.length))
            })
        })
        localEndpoint = client.localEndpoint
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
                text: qsTr("<ol><li style='list-style-position: outside'><b>Plug the Thymio you want to pair using the USB cable</b></li><br/><li style='list-style-position: outside'><b>Plug the USB dongle</b></li></ol>")
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
                enabled: dongles && dongles.length === 1
            }
            Button {
                text: qsTr("Advanced Mode")
            }
        }
    }
}

