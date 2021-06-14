import QtQuick 2.0

Item {
    id:item
    property var selectedApp
    property var selectedAppLauncher

    Rectangle {
        id: bg
        anchors.fill: parent
        color: Style.main_bg_color
    }
    Item {
        anchors.margins: Style.window_margin + Style.column_margin + Style.column_width
        anchors.fill: parent
        Column {
            anchors.fill: parent
            anchors.horizontalCenter: parent.horizontalCenter

            Image {
                id:thymio_logo
                source: "qrc:/assets/logo-thymio.svg"
                anchors.horizontalCenter: parent.horizontalCenter
                height: 73
                width : 206
                antialiasing: true
                smooth:true
                
            }

            Text {
                id: titleText
                anchors.horizontalCenter: parent.horizontalCenter
                textFormat: Text.RichText
                text: qsTr("<div align='center' style='font-size:24px'>Welcome to Thymio Suite</div><div style='font-size:16px'>Choose your programming language to 
                learn with Thymio</div>")
            }



            ListView {
                width:parent.width
                height:parent.height - titleText.height - thymio_logo.height
                model: Applications {}
                orientation: Qt.Horizontal
                id : app_view
                spacing: 0
                currentIndex: -1
                delegate: Item {
					anchors.topMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    height: icon.height + label.height + 12
                    width:  (app_view.width) / app_view.count

                    Column {
                        anchors.fill: parent
                        AnimatedImage {
                            id: icon
                            source: animatedIcon
                            playing: false


                            height: width
                            width : Math.max(48, Math.min(256, parent.width - 2 * 30))

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
                                    item.selectedApp = app_view.model.get(app_view.currentIndex)
                                    item.selectedAppLauncher = app_view.model.launch_function(item.selectedApp)
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
                    }
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
        text: qsTr("<a href=\"https://thymio.org\">Thymio Suite</a> - %1").arg(Qt.application.version)
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
