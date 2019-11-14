import QtQuick 2.11
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import org.mobsya  1.0

Item {
    id:selection_view
    property alias selectedDevice: device_view.selectedDevice


    function isThymio(device) {
        return device.type === ThymioNode.Thymio2
                || device.type === ThymioNode.Thymio2Wireless
                || device.type === ThymioNode.SimulatedThymio2
    }

    function launchSelectedAppWithSelectedDevice() {
        const device   = selection_view.selectedDevice
        const selectedAppLauncher = launcher.selectedAppLauncher;
        if(!selectedAppLauncher) {
            console.error("No launch function")
        }
        else if(!selectedAppLauncher(device)) {
            console.error("could not launch %1 with device %2".arg(selectedAppLauncher.name).arg(device))
        }
        if(!(device.status === ThymioNode.Available || selectedAppLauncher.supportsWatchMode)
                &&  (!device.isInGroup || selectedAppLauncher.supportsGroups)
                &&  (isThymio(device)  || selectedAppLauncher.supportsNonThymioDevices))
            device_view.selectedDevice = null
    }

    Rectangle  {
        id: app_titlebar
        anchors.top: parent.top
        width: parent.width
        height: Style.titlebar_height
        color: Style.dark
        Image {
            id: app_icon
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 15
            anchors.left: parent.left
            source: launcher.selectedApp.icon
            width : 30
            height: 30
            mipmap:true

        }

        Text {
            text: launcher.selectedApp.name
            color: "white"
            font.bold: true
            font.family: "Roboto"
            font.pointSize: 12
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 10
            anchors.left: app_icon.right
            smooth: true
            antialiasing: true
        }

        SvgButton {
            source: "qrc:/assets/launcher-icon-help-blue.svg"
            height: 22
            width : 22
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: icon_close.left
            anchors.rightMargin: (parent.height - height) / 2
            onClicked: {
                Qt.openUrlExternally(launcher.selectedApp.helpUrl.arg(Utils.uiLanguage))
            }
        }


        SvgButton {
            source: "qrc:/assets/launcher-icon-close.svg"
            height: 22
            width : 22
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: (parent.height - height) / 2
            id: icon_close
            onClicked: {
                launcher.goToAppSelectionScreen()
            }
        }

    }
    GridLayout {
        anchors.top: app_titlebar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        flow:  width > height ? GridLayout.LeftToRight : GridLayout.TopToBottom
        columnSpacing: 0
        rowSpacing: 0


        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            id: left_pane
            Rectangle {
                anchors.fill: parent
                color: Style.light

                Item {
                    anchors.fill: parent
                    Image {
                        source: launcher.selectedApp.descriptionImage
                        anchors.left:  parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        fillMode: Image.PreserveAspectCrop
                        verticalAlignment: Image.AlignTop
                        horizontalAlignment: Image.AlignLeft
                        height: {
                            return selection_view.width > selection_view.height ? Math.min(width * 0.5, parent.height* 0.5) : parent.height * 0.33
                        }

                        id: img_description
                    }

                    Item {
                        anchors.left:  parent.left
                        anchors.right: parent.right
                        anchors.top: img_description.bottom
                        anchors.bottom: parent.bottom
                        anchors.margins: Style.window_margin
                        ScrollView {
                            width: parent.width
                            height: parent.height
                            id: scrollview
                            clip: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                            Text {
                                 width: scrollview.width - 10
                                 id: description_text
                                 textFormat: Text.RichText
                                 text:  Utils.readFileContent(Utils.filenameForLocale(launcher.selectedApp.descriptionTextFile))
                                 wrapMode: Text.WordWrap
                                 onLinkActivated: Qt.openUrlExternally(link)

                                 MouseArea {
                                     anchors.fill: parent
                                     acceptedButtons: Qt.NoButton // we don't want to eat clicks on the Text
                                     cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                                 }
                            }
                        }
                    }
                }
            }
        }
        Rectangle {
            id: selection_pane
            Layout.maximumWidth:  {
                if(parent.width < parent.height)
                    return parent.width

                var mW = selection_view.width / 2
                var count = Math.floor(mW / 172)
                return Math.min(4, count) * 172  + 30 * 2
            }
            Layout.minimumWidth: Layout.maximumWidth
            Layout.fillHeight: true

            color: Style.mid
            anchors.bottomMargin: Style.window_margin
            anchors.topMargin: Style.window_margin

            Item {
                visible: Utils.isZeroconfRunning
                id: selection_title
                anchors.left: parent.left
                anchors.right: parent.right
                height: 30
                anchors.top: parent.top
                anchors.leftMargin: 30
                anchors.rightMargin: 30
                anchors.topMargin: 12
                Text {
                    anchors.centerIn: parent
                    text: qsTr("Connect a Thymio or <a href='#'>launch a simulator</a>")
                    color: "white"
                    linkColor: "#0a9eeb"
                    font.family: "Roboto Bold"
                    font.pointSize: 12
                    onLinkActivated: Utils.launchPlayground()
                    wrapMode: Text.WordWrap
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Rectangle {
                id:button
                anchors.horizontalCenter: parent.horizontalCenter
                height: 40;
                width : 220;
                radius: 20
                anchors.bottom: parent.bottom
                anchors.topMargin: Style.window_margin
                color: mouse_area.containsMouse ? "#57c6ff" : "#0a9eeb"
                Text {
                    font.family: "Roboto Bold"
                    font.pointSize: 12
                    color : "white"
                    anchors.centerIn: parent
                    text : qsTr("Launch %1").arg(launcher.selectedApp.name)

                }
                MouseArea {
                    enabled: !!(launcher.selectedApp && selection_view.selectedDevice)
                    anchors.fill: parent
                    hoverEnabled: true
                    id: mouse_area
                    cursorShape: Qt.PointingHandCursor
                    onClicked: launchSelectedAppWithSelectedDevice()
                }
                anchors.bottomMargin: 30
            }


            Item {
                id:grid_container
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top : selection_title.bottom
                anchors.bottom: button.top

                //color: 'cyan'
                anchors.topMargin: Style.window_margin
                anchors.bottomMargin: Style.window_margin


                anchors.leftMargin : 30
                anchors.rightMargin: anchors.leftMargin

                GridView {
                    property var selectedDevice
                    id: device_view
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: {
                        return cellWidth * Math.floor(grid_container.width / cellWidth)
                    }

                    height: parent.height
                    cellWidth : 172
                    cellHeight: 172
                    boundsBehavior: Flickable.StopAtBounds
                    clip:true

                    ScrollBar.vertical: ScrollBar { }

                    model: thymios

                    delegate:  ThymioSelectionDeviceDelegate { }
                }
            }
            Text {
                anchors.centerIn: parent
                visible: !Utils.isZeroconfRunning
                id: zeoconfErrorMessage
                text: {
                    if(Utils.platformIsLinux()) {
                        return qsTr("No robot found because the Avahi Daemon is missing or not running. <a href='http://google.com'>Troubleshooting</a>")
                    }
                    return qsTr("No robot found because the Bonjour service is missing or not running. <a href='https://www.thymio.org/faq/my-thymio-robot-does-not-appear-in-the-robot-selection-list/'>Troubleshooting</a>")
                }
                color: "#DE7459"
                linkColor: "#F9F871"
                font.family: "Roboto Bold"
                font.pointSize: 13
                onLinkActivated: Qt.openUrlExternally(link)
                wrapMode: Text.WordWrap
                width: parent.width * 0.90
            }
        }
    }
}
