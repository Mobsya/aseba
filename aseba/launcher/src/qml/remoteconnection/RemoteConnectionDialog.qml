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

    visible: true
    Rectangle
    {
        id: connectionzone
        anchors.top: parent.top
        width : parent.width
        height : parent.height*0.6
        color: Style.light
    }

    TitleBar {}

    property string ip   : ""
    property string ipv6 : ""

    function fontsize(){
        if(remoteConnection.height>=800)
            return 16;
        return 12;
    }


    Item {
        id: inputzone
        anchors.fill: parent
        anchors.margins:60
        anchors.topMargin: 80
        Item {
            width : parent.width / 2
            height : 120
            id: connectIP
            anchors.margins: 20
            TextEdit {
                id:connectIPText
                textFormat: TextEdit.RichText
                readOnly: true
                selectByMouse: true
                font.pointSize: fontsize()
                anchors.left: parent.left
                anchors.right: parent.right
                wrapMode: Text.WordWrap
                color: "white"
                clip: true
                font.family: "Roboto Bold"
                visible: Utils.platformHasSerialPorts()
                text: {
                    var explainText = qsTr("<b>Your address and password are below. This information is needed to enable other users to access your robot(s).<br/><br/>ADDRESS</b>")
                    var ipText = qsTr("<a href='https://whatismyipaddress.com/'>Click here to show your IP address</a>")
                    if(ip != "") {
                        ipText = qsTr("ipv4: <b>%1</b><br/>ipv6: <b>%2</b>")
                        .arg(ip)
                        .arg(ipv6)
                    }
                    var passwordText = qsTr("<b>PASSWORD</b>");
                    if(client && client.localEndpoint && client.localEndpoint.password !== "")
                        passwordText = qsTr("<b>PASSWORD</b><br/>%1")
                    .arg(client.localEndpoint.password)

                    return qsTr("%1<br/>%2<br/><br/>%3<br/><br/>Be sure your port 8596 and 8597 are open and redirect to this computer <a href='https://www.google.com/'>More information</a>")
                    .arg(explainText)
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
            anchors.top: parent.top
            anchors.right:parent.right
            anchors.margins: 20
            width: parent.width / 2.2
            height: parent.height * 0.55
            id: connectImage
            source : "qrc:/assets/remote_access.svg"
            anchors.topMargin: 0
            verticalAlignment: Image.AlignTop
            horizontalAlignment: Image.AlignHCenter
            transformOrigin: Item.Center
            sourceSize.height: connectImage.height
            sourceSize.width: connectImage.width
            fillMode: Image.PreserveAspectFit
            smooth: true
            antialiasing: true
        }

        Text {
            id:connectText
            anchors.topMargin:20 + parent.height * 0.6
            anchors.horizontalCenter:parent.horizontalCenter
            anchors.top: parent.top
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            textFormat: TextEdit.RichText
            font.pointSize: fontsize()
            width : parent.width
            wrapMode: Text.WordWrap
            font.bold: true
            color: "white"
            font.family: "Roboto Bold"
            text: qsTr("To connect to the robot(s) of another host, please enter the address and password in the fields below:")
        }
        RowLayout {
            id:connectInputs
            width: inputzone.width
            anchors.top: connectText.bottom
            anchors.margins: 20
            clip: false
            anchors.horizontalCenter:parent.horizontalCenter
            spacing: 15
            TextField  {
                color: "white"
                width: connectInputs.width - passwordInput.width - portInput.width - 150
                horizontalAlignment: Text.AlignLeft
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                placeholderTextColor: "#9effffff"
                font.pointSize: 14
                font.bold: true
                font.family: "Roboto Bold"
                background:Rectangle {
                    width: hostInput.Width
                    color: "#35216b"
                    radius: 5
                    border.width: 0
                }
                placeholderText: qsTr("ADDRESS")
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
                font.pointSize: 14
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
                font.pointSize: 14
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
        Text {
            id:warningText
            anchors.margins: 20
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            textFormat: TextEdit.RichText
            font.pointSize: 12
            anchors.horizontalCenter:parent.horizontalCenter
            anchors.top: connectInputs.bottom
            width : parent.width
            wrapMode: Text.WordWrap
            color: "white"
            font.family: "Roboto Bold"
            text: qsTr("The connection will not be encrypted. Do not connect to hosts you don't trust !")
        }
        Item {
            anchors.top: warningText.bottom
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
                enabled: {if(hostInput.text != "" && passwordInput.text != "")
                            return true;
                          return false}
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
            font.pointSize: 14
        }

    }

    function showErrorMessage(text) {
        message.visible = true
        message.text = qsTr("Error: %1 - Please verify the address, port and password of the server")
        .arg(text)
        message.color = "red"
    }

    function showMessage(text) {
        message.visible = true
        message.text = text
        message.color = "white"
    }

    function connect() {
        var host = hostInput.text
        var port = parseInt(portInput.text)
        var password = passwordInput.text
        var monitor = Utils.connectToServer(host, port, password)
        monitor.error.connect(function(msg) {
            showErrorMessage(msg)
            connectButtonid.enabled = true
        })
        monitor.message.connect(function(msg) {
            showMessage(msg)
            connectButtonid.enabled = false
        })
        monitor.done.connect(function() {
            message.visible = true
            message.text = qsTr("Connected!")
            message.color = "white"
            connectButtonid.enabled = true
        })

        showMessage(qsTr("Connecting...."))
        connectButtonid.enabled = false
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
    D{i:0;autoSize:true;height:560;width:960}
}
##^##*/
