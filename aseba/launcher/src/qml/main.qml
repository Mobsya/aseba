import QtQuick 2.6
import QtQuick.Controls 2.4

Item {
    id: launcher
    visible:true
    property var selectedApp : app_view.selectedApp

    BottomEasingStackView {
        id: main_layout
        anchors.fill: parent
        initialItem: ApplicationSelectionView {
            id: app_view
            onSelectedAppChanged: {
                goToDeviceSelectionWithApp()
            }
        }
    }

    Component {
        id: deviceSelectionViewFactory
        ThymioSelectionView {
            id: deviceSelectionView
        }
    }

    function goToDeviceSelectionWithApp() {
        if(app_view.selectedApp && main_layout.depth == 1) {
            var component = deviceSelectionViewFactory.createObject(main_layout)
            main_layout.push(component)
        }
    }

    function goToAppSelectionScreen() {
        if(main_layout.depth > 1) {
            main_layout.pop()
        }
    }
}