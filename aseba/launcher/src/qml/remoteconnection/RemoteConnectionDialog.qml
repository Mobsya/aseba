import QtQuick 2.3
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.5
import QtQuick.Controls.Styles 1.4
import "../components"
import ".."

Rectangle {
    id: remoteConnection
    color: Style.mid
    anchors.fill: parent

    /*
      Put a mouse area over all the interface to prevent the
      mouse events to propagate to the main launcher interface
      which lays underneath
    */
    MouseArea {
        anchors.fill: parent
    }

    TitleBar {}

    property string ip   : ""
    property string ipv6 : ""
    visible: true

    Item {
        id: inputzone
        anchors.fill: parent
        anchors.margins:60

        Item {
            width : parent.width
            height : 120
            id: connectIP
            anchors.margins: 20
            TextEdit {
                id:connectIPText
                textFormat: TextEdit.RichText
                readOnly: true
                selectByMouse: true
                font.pointSize: 14
                anchors.left: parent.left
                anchors.right: parent.right
                wrapMode: Text.WordWrap
                color: "white"
                clip: true
                font.family: "Roboto Bold"
                text: {
                    var ipText = qsTr("<a href='https://whatismyipaddress.com/'>Click here to show your IP address</a>")
                    if(ip != "") {
                        ipText = qsTr("The address of this host is:<br/>  - ipv4: <b>%1</b><br/>  - ipv6: <b>%2</b>")
                        .arg(ip)
                        .arg(ipv6)
                    }
                    var passwordText = "";
                    if(client && client.localEndpoint && client.localEndpoint.password !== "")
                        passwordText = qsTr("<br/>Password for remote connections: <b>%1</b>")
                    .arg(client.localEndpoint.password)

                    return qsTr("%1<br/>%2")
                    .arg(ipText)
                    .arg(passwordText)
                }
                onLinkActivated: Qt.openUrlExternally(link)
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }

            }
        }

        Image {
            anchors.top: connectIP.bottom
            width: parent.width
            height: Math.min(parent.height-370,400)
            id: connectImage
            verticalAlignment: Image.AlignTop
            source : "qrc:/assets/remote_access.svg"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 20
            transformOrigin: Item.Center
            sourceSize.height: connectImage.height
            sourceSize.width: connectImage.width
            fillMode: Image.PreserveAspectFit
            horizontalAlignment: Image.AlignHCenter
            smooth: true
            antialiasing: true
        }

        Text {
            id:connectText
            anchors.margins: 30
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            textFormat: TextEdit.RichText
            font.pointSize: 16
            anchors.horizontalCenter:parent.horizontalCenter
            anchors.top: connectImage.bottom
            width : parent.width
            wrapMode: Text.WordWrap
            color: "white"
            font.family: "Roboto Bold"
            text: qsTr("Input the address of a server to connect to<br/> The connection will not be encrypted. Do not connect to hosts you don't trust !")
        }
        RowLayout {
            id:connectInputs
            width: inputzone.width
            anchors.top: connectText.bottom
            anchors.margins: 30
            clip: false
            anchors.horizontalCenter:parent.horizontalCenter
            spacing: 15
            TextField  {
                color: "white"
                width: connectInputs.width - passwordInput.width - portInput.width - 150
                horizontalAlignment: Text.AlignLeft
                font.weight: Font.Normal
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                placeholderTextColor: "#9effffff"
                font.bold: true
                font.family: "Roboto Bold"
                font.pointSize: 15
                background:Rectangle {
                    width: hostInput.Width
                    color: "#35216b"
                    radius: 5
                    border.width: 0
                }
                placeholderText: qsTr("Host Address")
                id: hostInput
            }
            TextField  {
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                Layout.fillWidth: false
                placeholderTextColor: "#9effffff"
                font.capitalization: Font.AllUppercase
                width: 150
                font.pointSize: 15
                font.bold: true
                font.family: "Roboto Bold"
                background:Rectangle {
                    width: passwordInput.Width
                    color: "#35216b"
                    radius: 5
                    border.width: 0
                }
                validator: RegExpValidator {  regExp: /[0-9A-Z]{6}/ }
                maximumLength: 6
                placeholderText: qsTr("Password")
                id: passwordInput
            }
            TextField  {
                color: "white"
                placeholderTextColor: "#9effffff"
                font.capitalization: Font.AllUppercase
                font.pointSize: 15
                font.bold: true
                font.family: "Roboto Bold"
                width: 90
                background:Rectangle {
                    width: portInput.Width
                    color: "#35216b"
                    radius: 5
                    border.width: 0
                }
                validator: IntValidator { bottom: 1; top: 0xFFFF }
                maximumLength: 5
                text: "8596"
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                Layout.fillWidth: false
                placeholderText: qsTr("Port")
                id: portInput
            }
        }

        Item {
            id: item2
            anchors.top: connectInputs.bottom
            anchors.horizontalCenter: parent
            anchors.margins: 50
            width: parent.width

            Button {
                id: connectButtonid
                text: qsTr("Connect")
                onClicked: connect()
                width: parent.width / 3
                anchors.centerIn: parent
                anchors.top: parent.top
            }
        }
    }
    Rectangle {
        property alias text: message.text
        visible: message.text != ""
        id: errorBanner
        anchors.bottom: remoteConnection.bottom
        anchors.right: remoteConnection.right
        anchors.left: remoteConnection.left
        height: 40
        color: "#0a9eeb"
        Text {
            anchors.margins: 0
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            id: message
            visible: false
            color: "white"
            anchors.fill: parent
            font.family: "Roboto Bold"
        }

    }

    function showErrorMessage(text) {
        message.visible = true
        message.text = qsTr("Error: %1 - Please verify the address, port and password of the server")
        .arg(text)
        message.color = "red"
        message.font.pointSize= 12
    }

    function showMessage(text) {
        message.visible = true
        message.text = text
        message.color = "white"
        message.font.pointSize= 12
    }

    function connect() {
        var host = hostInput.text
        var port = parseInt(portInput.text)
        var password = passwordInput.text
        var monitor = Utils.connectToServer(host, port, password)
        monitor.error.connect(function(msg) {
            showErrorMessage(msg)
            button.enabled = true
        })
        monitor.message.connect(function(msg) {
            showMessage(msg)
            button.enabled = false
        })
        monitor.done.connect(function() {
            message.visible = true
            message.text = qsTr("Connected!")
            message.color = "green"
            button.enabled = true
            message.font.pointSize= 12
        })

        showMessage(qsTr("Connecting...."))
        button.enabled = false
        monitor.start()
    }

    function request(url, callback) {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = (function(myxhr) {
            return function() {
                callback(myxhr);
            }
        })(xhr);
        xhr.open('GET', url, true);
        xhr.send('');
    }

    Component.onCompleted: {
        request("https://api6.ipify.org", function(req) {
            ipv6 = req.responseText
        });
        request("https://api.ipify.org", function(req) {
            ip = req.responseText
        })
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:800;width:1024}
}
##^##*/
