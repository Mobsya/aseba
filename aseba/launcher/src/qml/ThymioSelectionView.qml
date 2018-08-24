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

                    delegate:  Item {
                        id:item
                        height: 172
                        width : 172
                        property bool selected: device_view.currentIndex == index
                        opacity: {
                            switch(status) {
                            case ThymioNode.Ready:
                            case ThymioNode.Available:
                                    return selected ? 1 : 0.8;
                            default: return 0.5
                            }
                        }

                        MouseArea {
                            id: device_mouse_area
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked:  {
                                device_view.currentIndex = index
                                //item.selected = device_view.currentIndex == index
                            }

                        }

                        Rectangle {
                            id: selectable_area
                            color: device_mouse_area.containsMouse ? "#353535" : "transparent"
                            border.width: item.selected ? 1.3333 : 0
                            border.color: "#0a9eeb"
                            anchors.fill: parent
                            anchors.horizontalCenter: parent.horizontalCenter

                            Column {
                                width : 142
                                height: 120
                                anchors.centerIn: parent

                                Item {
                                    width :  90
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    //color : "#FF00FF"
                                    height: 12


                                    BatteryIndicator {
                                        anchors.verticalCenter: parent.verticalCenter
                                        height: 8
                                        anchors.left: parent.left
                                        anchors.leftMargin: 2

                                    }

                                    Image {
                                         id: icon;
                                         source : "qrc:/assets/update-icon.svg"
                                         fillMode: Image.PreserveAspectFit
                                         anchors.verticalCenter: parent.verticalCenter
                                         height: 12
                                         anchors.right: parent.right
                                         anchors.leftMargin: 2
                                         smooth: true
                                         antialiasing: true
                                    }
                                }

                                Item {
                                    width: parent.width
                                    height: 8
                                }

                                Image {
                                    source: {
                                        switch (type) {
                                         case ThymioNode.Thymio2:
                                         case ThymioNode.Thymio2Wireless:
                                            return "qrc:/assets/thymio.svg"
                                         case ThymioNode.SimulatedThymio2:
                                            return "qrc:/assets/simulated_thymio.svg"
                                         default:
                                            return "qrc:/assets/dummy_node.svg"
                                        }
                                    }
                                    width :  90
                                    fillMode:Image.PreserveAspectFit
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Item {
                                    width: parent.width
                                    height: 20
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: name
                                    font.family: "Roboto"
                                    font.bold: true
                                    font.pointSize: 9
                                    color : "white"
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
