import QtQuick 2.12
import QtQuick.Dialogs 1.3

MessageDialog {
    property string oldVersion
    property string newVersion
    property string deviceName

    id: messageDialog
    title: qsTr("Open the Wireless Thymio network configurator?")
    text : qsTr("All Wireless Thymio robots associated to the Wireless dongle plugged on this computer will be disconnected until the Wireless Thymio network configuration window is closed.\nContinue?")
    icon: StandardIcon.Question
    standardButtons : StandardButton.Yes | StandardButton.No
}