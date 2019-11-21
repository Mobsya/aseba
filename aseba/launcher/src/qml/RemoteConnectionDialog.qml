import QtQuick 2.3
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.5
import QtQuick.Controls.Styles 1.4

Dialog {
    property string ip   : "<??>"
    property string ipv6 : "<??>"
    anchors.centerIn: parent
    visible: true
    title: qsTr("Connect to a remote server")
    standardButtons: Dialog.Close
    id:dialog
    contentItem: Item {
        Column {
            TextEdit {
                textFormat: TextEdit.RichText
                readOnly: true
                selectByMouse: true
                text: qsTr("The address of this host is:<br/>  - ipv4: <b>%1</b><br/>  - ipv6: <b>%2</b>").arg(ip).arg(ipv6)
            }

            Item {
                height: 10
                width: parent.width
            }
            Rectangle {
                height: 2
                width: parent.width
                color: "grey"
                border.color: "grey"
                border.width: 1
            }

            Item {
                height: 30
                width: parent.width
            }

            Text {
                text: qsTr("Input the address of a server to connect to<br/> <b> The connection will not be encrypted.<br/>Do not connect to host you don't trust !</b>")
            }
            Item {
                height: 10
                width: parent.width
            }
            RowLayout {
                spacing: 5
                TextField  {
                    Layout.minimumWidth: 80
                    Layout.fillWidth: true
                    placeholderText: qsTr("Host Address")
                    id: hostInput
                }
                TextField  {
                    validator: IntValidator { bottom: 1; top: 0xFFFF }
                    maximumLength: 6
                    placeholderText: qsTr("Port")
                    id: portInput
                }
                Button {
                    Layout.minimumWidth: 80
                    Layout.fillWidth: true
                    text: qsTr("Connect")
                    onClicked: connect()
                    id: button
                }
            }
            Item {
                height: 10
                width: parent.width
            }
            Text {
                id: message
                visible: false
            }
        }
    }

    function showErrorMessage(text) {
        message.visible = true
        message.text = qsTr("Error: ")  + text
        message.color = "red"
    }

    function showMessage(text) {
        message.visible = true
        message.text = text
        message.color = "black"
    }

    function connect() {
        var host = hostInput.text
        var port = parseInt(portInput.text)
        var monitor = Utils.connectToServer(host, port)
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