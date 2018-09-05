import QtQuick 2.11
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import org.mobsya  1.0

Item {
    id:selection_view
    Rectangle  {
        id: app_titlebar
        anchors.top: parent.top
        width: parent.width
        height: Style.titlebar_height
        color: Style.titlebar_bg_color
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
            source: "qrc:/assets/vertical-close.svg"
            height: 12
            width : 22
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: (parent.height - height) / 2
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
                color: Style.app_descrition_bg_color

                Item {
                    anchors.fill: parent
                    Rectangle {
                        color: 'magenta'
                        anchors.left:  parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        height: {
                            return selection_view.width > selection_view.height ? Math.min(width * 0.5, parent.height* 0.5) : parent.height * 0.33
                        }

                        id: img_description
                    }

                    Item {
                        //color: 'yellow'
                        anchors.left:  parent.left
                        anchors.right: parent.right
                        anchors.top: img_description.bottom
                        anchors.bottom: parent.bottom
                        anchors.margins: Style.window_margin
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

            color: Style.thymio_selection_bg_color
            anchors.bottomMargin: Style.window_margin
            anchors.topMargin: Style.window_margin

            Rectangle {
                id: selection_title
                anchors.left: parent.left
                anchors.right: parent.right
                height: 30
                anchors.top: parent.top
                anchors.leftMargin: 30
                anchors.rightMargin: 30
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
                    anchors.fill: parent
                    hoverEnabled: true
                    id: mouse_area
                    cursorShape: Qt.PointingHandCursor
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
                    id: device_view
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: {
                        console.log(grid_container.width, cellWidth, Math.floor(grid_container.width / cellWidth), cellWidth * (grid_container.width / cellWidth))
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
        }
    }
}
