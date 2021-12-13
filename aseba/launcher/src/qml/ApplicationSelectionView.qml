import QtQuick 2.12

Item {
    id:item1
    property var selectedApp
    property var selectedAppLauncher

    Rectangle {
        id: bg
        anchors.fill: parent
        color: Style.main_bg_color
    }
    Item {
        anchors.margins: 4 * Style.window_margin
        anchors.fill: parent
        anchors.horizontalCenter: parent.horizontalCenter
        Image {
            id:thymio_logo
            source: "qrc:/assets/logo-thymio.svg"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top:parent.top
            height: 73
            width : 206
            antialiasing: true
            smooth:true
        }
        Text {
            id: titleText
            width:parent.width
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.margins: Style.window_margin
            anchors.top: thymio_logo.bottom
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            textFormat: Text.RichText
            text: qsTr("<div align='center' style='font-size:24px'>Welcome to Thymio Suite</div><div style='font-size:16px'>Choose your programming language to learn with Thymio</div>")
        }
        ListView {
            id : app_view
            width:parent.width
            anchors.horizontalCenter: parent.horizontalCenter
            height:parent.height - titleText.height - thymio_logo.height - 2*Style.window_margin
            model: Applications {}
            orientation: Qt.Horizontal
            spacing: 0
            currentIndex: -1
            anchors.top:titleText.bottom
            anchors.margins: Style.window_margin
            delegate: Item {
                height: icon.height + label.height + 12
                width:  (app_view.width) / app_view.count
                anchors.verticalCenter: parent.verticalCenter
                Column {
                    anchors.fill: parent
                    AnimatedImage {
                        id: icon
                        source: animatedIcon
                        playing: false
                        height: width
                        width : Math.max(48, Math.min(app_view.height, parent.width - 2 * 30,256))

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: {
                                icon.playing = true
                            }
                            onClicked: {
                                //Make sure the index changed signal is always emitted on click
                                app_view.currentIndex = -1
                                app_view.currentIndex = index
                                item1.selectedApp = app_view.model.get(app_view.currentIndex)
                                item1.selectedAppLauncher = app_view.model.launch_function(item1.selectedApp)
                            }
                            cursorShape: Qt.PointingHandCursor
                        }
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Item {
                        height: 12
                        width: parent.width
                    }
                    Text {
                        id: label
                        anchors.horizontalCenter: icon.horizontalCenter
                        text: name
                    }
                anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    WindowButtons {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: Style.window_margin
        anchors.rightMargin: Style.window_margin
    }

    Text {
        id: version
        text: qsTr("<a href=\"https://thymio.org\">Thymio Suite</a> - %1 %2 %3")
             .arg(Qt.application.version)
             .arg(client.localEndpoint?
              qsTr("<br/> Local connection ") : "<br/>")
             .arg(Utils.isZeroconfRunning?
              qsTr("Discovery service enabled") : "")
        onLinkActivated: Qt.openUrlExternally(link)
        font.family: "Roboto Light"
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.bottomMargin: Style.window_margin
        anchors.leftMargin: Style.window_margin
        color: "#333"
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;formeditorZoom:0.6600000262260437;height:640;width:1024}
}
##^##*/
