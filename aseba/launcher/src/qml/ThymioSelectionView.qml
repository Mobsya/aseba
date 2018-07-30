import QtQuick 2.0
import QtQuick.Controls 2.3

Item {
    id:selection_view
    anchors.fill: parent
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
    Item {
        id: left_pane
        anchors.top:app_titlebar.bottom
        anchors.left: parent.left
        anchors.right: {
            return parent.width > parent.height ? selection_pane.left : parent.right
        }
        
        height: {
            return (parent.width > parent.height ? parent.height : parent.height / 2) - app_titlebar.height
        }
              
        Rectangle {
            anchors.fill: parent
            color: "orange" // Style.app_descrition_bg_color
            
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
                
                Rectangle {
                    color: 'yellow'
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
        width:  {
            if(parent.width < parent.height)
                return parent.width
            
            var mW = selection_view.width / 2
            var count = Math.floor(mW / 172)
            return Math.min(4, count) * 172  + 30
        }
        anchors.top: {
            return (parent.width > parent.height ) ? app_titlebar.bottom : left_pane.bottom
        }
        
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        
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
            anchors.left: parent.left
            anchors.right: parent.right
            height: 40;
            anchors.bottom: parent.bottom
            anchors.topMargin: Style.window_margin
            color: "blue"
        }
        
        
        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top : selection_title.bottom
            anchors.bottom: button.top
            
            //color: 'cyan'
            anchors.topMargin: Style.window_margin
            anchors.bottomMargin: Style.window_margin
            
            
            anchors.leftMargin: {
                var margin = (parent.width - 30) % 172
                return margin / 2
                
            }
            anchors.rightMargin: anchors.leftMargin
            
            GridView {
                anchors.fill: parent
                cellWidth : 172
                cellHeight: 172
                boundsBehavior: Flickable.StopAtBounds
                clip:true
                
                ScrollBar.vertical: ScrollBar { }
                
                model: ListModel {
                    ListElement {
                        name : "thymio 1"
                    }
                    ListElement {
                        name : "thymio 2"
                    }
                    ListElement {
                        name : "thymio 3"
                    }
                    ListElement {
                        name : "thymio 4"
                    }
                    ListElement {
                        name : "thymio 5"
                    }
                    ListElement {
                        name : "thymio 6"
                    }
                    ListElement {
                        name : "thymio 7"
                    }
                    ListElement {
                        name : "thymio 8"
                    }
                    ListElement {
                        name : "thymio 9"
                    }
                    ListElement {
                        name : "thymio 10"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                    ListElement {
                        name : "thymio 11"
                    }
                }
                
                delegate:  Item {
                    height: 172
                    width : 172
                    //border.width: 1
                    //border.color: "green"
                    
                    Column {
                        width : 142
                        height: 142 
                        anchors.horizontalCenter: parent.horizontalCenter
                        Image {
                            source: "qrc:/assets/physical_thymio.svg"
                            width :  90
                            height:  90
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Item {
                            id: margin
                            width: parent.width
                            height: 10
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr(name)
                            font.family: "Roboto Light"
                            font.pointSize: 12
                            color : "white"
                        }
                    }
                }
            }
        }
    }
}
