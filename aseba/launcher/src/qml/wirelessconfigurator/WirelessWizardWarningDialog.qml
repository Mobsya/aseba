import QtQuick 2.2
import QtQuick.Dialogs 1.1

MessageDialog {
    property string oldVersion
    property string newVersion
    property string deviceName

    id: messageDialog
    title: qsTr("Open the wireless configurator?")
    text : qsTr("All wireless robots associated to a dongle plugged on this computer will be disconnected until the wireless configuration window is closed.\nContinue?")
    icon: StandardIcon.Question
    standardButtons : StandardButton.Yes | StandardButton.No
}