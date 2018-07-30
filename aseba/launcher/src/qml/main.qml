import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.3

ApplicationWindow {
    id: main_window
    title: qsTr("Thymio Launcher")
    visible:true
    minimumHeight: 640
    minimumWidth:  1024
    Rectangle {
        id: bg
        anchors.fill: parent
        color: Style.main_bg_color
    }
    
    StackLayout {
        id: main_layout
        currentIndex: 0
        
        anchors.fill: parent
        ApplicationSelectionView {
            anchors.fill: parent
            id: app_view
        }
        ThymioSelectionView {
            anchors.fill: parent
            id: thymio_view
        }
        
        onCurrentIndexChanged: {
            if(thymio_view.visible) {
                bg.color = Style.thymio_selection_bg_color
            }
            else {
                bg.color = Style.main_bg_color
            }
        }
    }
    
    Button {
        anchors.top: main_window.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: "toogle"
        onClicked: main_layout.currentIndex = main_layout.currentIndex == 1 ? 0 : 1
    }
}
