import QtQuick 2.0

Item {
    anchors.fill: parent
    
    ListView {
        anchors.margins: Style.window_margin + Style.column_margin + Style.column_width
        anchors.fill: parent
        model: Applications {}
        orientation: Qt.Horizontal
        id : app_view
        spacing: 0
        
//        Rectangle {
//            anchors.fill: parent
//            opacity: 0.1
//            border.color: "black"
//            border.width: 2
//        }

        delegate: Item {
            anchors.verticalCenter: parent.verticalCenter 
            height: icon.height + label.height + 12
            width:  (app_view.width) / app_view.count
            
//            Rectangle {
//                anchors.fill: parent
//                opacity: 0.1
//                border.color: "magenta"
//                border.width: 2
//            }   
            
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
    
    WindowButtons {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: Style.window_margin
        anchors.rightMargin: Style.window_margin
    }
    
    Text {
        id: version
        text: qsTr("Version 42 - lorem ipsum yo")
        font.family: "Roboto Light"
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.bottomMargin: Style.window_margin
        anchors.leftMargin: Style.window_margin
        color: "#333"
    }
}
