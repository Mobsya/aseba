import QtQuick 2.12
import QtQuick.Dialogs 1.1

MessageDialog {
    property string oldVersion
    property string newVersion
    property string deviceName

    id: messageDialog
    title: qsTr("Update Firmware?")
    text : qsTr("Install Firmware %1 on %2?")
        .arg(newVersion).arg(deviceName)
    informativeText: qsTr("This should only take a few seconds.\nDo not unplug the device while the update is in progress.")
    icon: StandardIcon.Question
    standardButtons : StandardButton.Yes | StandardButton.No
}