import QtQuick 2.0
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
        Row {
            anchors.fill: parent
            anchors.leftMargin: 30
            Text {
                id: test
                text: qsTr("Hello")
                color: "white"
                font.bold: false
                font.family: "Roboto"
                font.pointSize: 20
                anchors.verticalCenter: parent.verticalCenter
            }
        }

    }
    GridLayout {
        anchors.top: app_titlebar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        flow:  width > height ? GridLayout.LeftToRight : GridLayout.TopToBottom


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
                    text : qsTr("Launch Blockly")

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
