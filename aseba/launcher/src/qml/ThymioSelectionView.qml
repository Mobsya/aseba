import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import org.mobsya  1.0

Item {
    id:selection_view
    property alias selectedDevice: device_view.selectedDevice


    function isThymio(device) {
        return device.type === ThymioNode.Thymio2
                || device.type === ThymioNode.Thymio2Wireless
                || device.type === ThymioNode.SimulatedThymio2
    }


    /* ****************************
     *   Given access to the local current view, the function retrieves the one or several devices selected by the user. If the selected app supports watch functionalities and the device is Busy, is considered to be Ready
     *   return: boolean
     *      true if
     *          the device is ready and fully binded
     *      false  otherwise
     *  2do: add group management - add non thymio devices support
     **************************** */
    function isSelectedDeviceReady() {
        var the_device = selection_view.selectedDevice
        if (the_device == null)
            return false
        if( launcher.selectedApp.supportsWatchMode && the_device.status === ThymioNode.Busy  )
            return true
        if(the_device.status === ThymioNode.Available ||  the_device.status === ThymioNode.Ready )
            return true
        return false
    }

    function isSelectedApplicationInstalled() {
        return Utils.isApplicationInstalled(launcher.selectedApp.appId);
    }

    function isAppNotOther(){
         if(launcher.selectedApp.appId == "other")
             return false;
         else
              return true;
    }

    function launchSelectedAppWithSelectedDevice() {
        const device   = selection_view.selectedDevice
        const selectedAppLauncher = launcher.selectedAppLauncher;
        if(!selectedAppLauncher) {
            console.error("No launch function")
            return
        }

        if(launcher.selectedApp.appId == "studio" ){
            if( isSelectedDeviceReady())
                if(!selectedAppLauncher(device))
                    console.error("could not launch %1 with device %2".arg(selectedAppLauncher.name).arg(device))
        }
        else{
             if(!selectedAppLauncher(device))
                console.error("could not launch %1 with device %2".arg(selectedAppLauncher.name).arg(device))
        }
        if(!(device.status === ThymioNode.Available || selectedAppLauncher.supportsWatchMode)
                &&  (!device.isInGroup || selectedAppLauncher.supportsGroups)
                &&  (isThymio(device)  || selectedAppLauncher.supportsNonThymioDevices))
            device_view.selectedDevice = null
    }

    function openInstallationURLForSelectedApp() {
        if (launcher.selectedApp.installUrl != "") {
            Qt.openUrlExternally(launcher.selectedApp.installUrl);
        }
    }

    /* ****************************
     *   The function set the content for the id:button button depending on if the launched app supports watch functionality - in this case and if is there a watchable device available the string would differs
     *   return: translatable string
     *      "Watch with APPNAME" if we support watchable and the device is ready as defined in isSelectedDeviceReady()
     *      "Program with APPNAME" otherwise
    ******************************* */
    function launchButtonText(){
        if (!isSelectedApplicationInstalled()) {
            return qsTr("Install %1").arg(launcher.selectedApp.name)
        }
        if(launcher.selectedApp.supportsWatchMode && selection_view.selectedDevice
                && selection_view.selectedDevice.status === ThymioNode.Busy){
            return qsTr("Watch with %1").arg(launcher.selectedApp.name)
        }
        return qsTr("Program with %1").arg(launcher.selectedApp.name)
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
                            return selection_view.width > selection_view.height ? Math.min(width * 0.66, parent.height* 0.5) : parent.height * 0.5
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
                            contentWidth: description_text.width + 10
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
                visible: Utils.isZeroconfRunning && Utils.isPlaygroundAvailable
                    && isSelectedApplicationInstalled()
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
                    font.family: "Roboto"
                    font.pointSize: 12
                    onLinkActivated: Utils.launchPlayground()
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton // we don't want to eat clicks on the Text
                        cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                    wrapMode: Text.WordWrap
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Item {
                visible: Utils.isZeroconfRunning && (Utils.platformIsIos()||Utils.platformIsAndroid()) && device_view.count == 0
                id: selection_warning
                anchors.left: parent.left
                anchors.right: parent.right
                height: 30
                anchors.top: parent.top
                anchors.leftMargin: 30
                anchors.rightMargin: 30
                anchors.topMargin: 12
                Text {
                    anchors.centerIn: parent
                    text: qsTr("Make Sure Thymio Suite is launched on a relay computer. <a href='https://www.thymio.org/faq/how-to-program-a-thymio-with-a-tablet/'>More Informations</a>")
                    color: "white"
                    linkColor: "#0a9eeb"
                    font.family: "Roboto"
                    font.pointSize: 12
                    onLinkActivated: Qt.openUrlExternally(link)
                    wrapMode: Text.WordWrap
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                }
            }


            Rectangle {
                id:button

                height: 40
                width : 250
                radius: 20

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.topMargin: Style.window_margin
                anchors.bottomMargin: 30
                visible:isAppNotOther()
                color: mouse_area.containsMouse ? "#57c6ff" : "#0a9eeb"
                opacity: isSelectedDeviceReady() || !isSelectedApplicationInstalled()
                    ? 1.0
                    : 0.3
                Text {
                    font.family: "Roboto"
                    font.pointSize: 12
                    color : "white"
                    anchors.centerIn: parent
                    text : launchButtonText()
                }
                MouseArea {
                    enabled: isSelectedDeviceReady() || !isSelectedApplicationInstalled()
                    anchors.fill: parent
                    hoverEnabled: true
                    id: mouse_area
                    cursorShape: isSelectedDeviceReady() || !isSelectedApplicationInstalled()
                        ? Qt.PointingHandCursor
                        : Qt.ForbiddenCursor
                    onClicked: isSelectedApplicationInstalled()
                        ? launchSelectedAppWithSelectedDevice()
                        : openInstallationURLForSelectedApp()
                }

            }

            Item {
                id:grid_container
                visible: isSelectedApplicationInstalled()
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

                    ScrollBar.vertical: ScrollBar {
                        policy: device_view.contentHeight > 0.9 * device_view.height ?
                                    ScrollBar.AlwaysOn : ScrollBar.AsNeeded

                    }

                    model: thymios

                    delegate:  ThymioSelectionDeviceDelegate { }
                }
            }
            Text {
                anchors.centerIn: parent
                visible: !isSelectedApplicationInstalled()
                id: installationMessage
                text: {
                    return qsTr(Utils.platformIsLinux()
							? "%1 must be installed and started outside Thymio Suite."
							: "%1 is not installed on your computer.")
                        .arg(launcher.selectedApp.name)
                }
                color: "white"
                font.family: "Roboto"
                font.pointSize: 14
                wrapMode: Text.WordWrap
                width: parent.width * 0.90
            }
            Text {
                anchors.centerIn: parent
                visible: !Utils.isZeroconfRunning && device_view.count == 0 && !client.localEndpoint
                id: zeoconfErrorMessage
                text: {
                    if(Utils.platformIsLinux()) {
                        return qsTr("No robot found because the Avahi Daemon is missing or not running. <a href='https://www.thymio.org/faq/my-thymio-robot-does-not-appear-in-the-robot-selection-list/'>Troubleshooting</a>")
                    }
                    return qsTr("No robot found because the Bonjour service is missing or not running. <a href='https://www.thymio.org/faq/my-thymio-robot-does-not-appear-in-the-robot-selection-list/'>Troubleshooting</a>")
                }
                color: "#DE7459"
                linkColor: "#F9F871"
                font.family: "Roboto"
                font.pointSize: 13
                onLinkActivated: Qt.openUrlExternally(link)
                wrapMode: Text.WordWrap
                width: parent.width * 0.90
            }
        }
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/
