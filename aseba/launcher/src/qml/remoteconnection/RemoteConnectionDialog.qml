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
        anchors.fill: parent
        anchors.margins:90

        Item {
            width : parent.width
            anchors.top:parent
            anchors.margins: 20
            TextEdit {
                id: connectIP
                textFormat: TextEdit.RichText
                readOnly: true
                selectByMouse: true
                font.pointSize: 15
                anchors.left: parent.left
                anchors.right: parent.right
                wrapMode: Text.WordWrap
                color: "white"
                clip: true
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

        Item {
            id: connectImage
            height: parent.height/2
            width : parent.width
            anchors.top: connectIP.bottom

            Image {
                 source : "qrc:/assets/usb-cable-and-dongle.svg"
                 fillMode: Image.PreserveAspectFit
                 anchors.centerIn: parent
                 smooth: true
                 antialiasing: true
            }
        }
        Text {
            id:connectText
            anchors.margins: 20
            textFormat: TextEdit.RichText
            font.pointSize: 12
            anchors.horizontalCenter:parent.horizontalCenter
            anchors.top: connectImage.bottom
            width : parent.width
            wrapMode: Text.WordWrap
            color: "white"
            text: qsTr("Input the address of a server to connect to<br/> <b> The connection will not be encrypted. Do not connect to hosts you don't trust !</b>")
        }
        RowLayout {
            id:connectInputs
            anchors.top: connectText.bottom
            anchors.margins: 20
            anchors.horizontalCenter:parent.horizontalCenter
            spacing: 10
            TextField  {
                Layout.minimumWidth: 100
                Layout.fillWidth: true
                placeholderText: qsTr("Host Address")
                id: hostInput
            }
            TextField  {
                validator: IntValidator { bottom: 1; top: 0xFFFF }
                maximumLength: 6
                text: "8596"
                placeholderText: qsTr("Port")
                id: portInput
            }

            TextField  {
                validator: RegExpValidator {  regExp: /[0-9A-Z]{6}/ }
                maximumLength: 6
                placeholderText: qsTr("Password")
                id: passwordInput
            }
        }

        Item {
            anchors.top: connectInputs.bottom
            anchors.horizontalCenter: parent
            anchors.margins: 40
            width: parent.width

            Button {
                id: connectButtonid
                text: qsTr("Connect")
                onClicked: connect()
                width: parent.width / 3
                anchors.centerIn: parent
                anchors.top: parent.top
            }
            Item{
                anchors.top: connectButtonid.bottom
                anchors.horizontalCenter: connectButtonid.horizontalCenter
                anchors.margins: 20
                Text {
                    anchors.margins: 20
                    anchors.top: parent.top
                    id: message
                    visible: false
                }

            }
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
    D{i:0;autoSize:true;formeditorZoom:0.66;height:960;width:1280}
}
##^##*/
