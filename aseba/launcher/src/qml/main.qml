import QtQuick 2.12
import QtQuick.Controls 2.4
import "wirelessconfigurator"

Item {
    id: launcher
    visible:true
    property var selectedApp : app_view.selectedApp
    property var selectedAppLauncher : app_view.selectedAppLauncher

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

    SettingsMenu {
        id: settingsMenu
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        visible: false
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