import QtQuick.Controls 2.2
import QtQuick.Controls.Styles 1.4
import QtGraphicalEffects 1.0
import QtQuick 2.11


TextField {
    property string deviceName
    property bool editable: true

    onDeviceNameChanged: {
        text = deviceName
    }

    //anchors.horizontalCenter: parent.horizontalCenter
    font.family: "Roboto"
    font.bold: readOnly
    font.pointSize: 9
    color : "white"
    selectionColor: "#0a9eeb"
    readOnly: true
    maximumLength: 20
    wrapMode: Text.WordWrap

    FontMetrics {
        id: fontMetrics
        font: parent.font
    }

    TextMetrics {
        id: textMetrics
        font: parent.font
        text: deviceName
    }

    onTextChanged: {
        font.pointSize = 12
        FontMetrics.font = font
        while(fontMetrics.boundingRect(text).width >= width - 10) {
            font.pointSize = font.pointSize - 0.1
            fontMetrics.font = font
            if(font.pointSize < 5)
                break
        }
    }

    onFocusChanged: {
        if(!focus)
            readOnly = true
    }
    selectByMouse: true

    Keys.onPressed: {
        if (!readOnly && event.key === Qt.Key_Return || event.key === Qt.Key_Escape) {
            event.accepted = true;
            readOnly = true
        }
    }

    MouseArea {
        anchors.fill: parent
        onDoubleClicked: {
            if(editable)
                readOnly = false
        }
        enabled: editable && readOnly
    }

    onReadOnlyChanged: {
        if(!readOnly) {
            selectAll()
            forceActiveFocus()
        }
        else {
            if(text) {
                textEdited()
                accepted()
            }
            text = deviceName
        }
    }

    InverseMouseArea {
        enabled: !readOnly
        onPressed:  {
            readOnly = true
        }
        anchors.fill: parent
    }

    background: Item {
        anchors.margins: 0
        anchors.fill: parent
        RectangularGlow {
            id: effect
            anchors.fill: glowing
            glowRadius: 3
            spread: 0.1
            color: "#0a9eeb"
            cornerRadius: glowRadius
            enabled: !readOnly
            visible: !readOnly
        }


        Rectangle {
            color: Style.light
            anchors.centerIn: parent
            anchors.fill: parent
            id:glowing
            visible: !readOnly
            radius: 2
        }

        Rectangle {
            color: readOnly ? "transparent" : Style.light
            radius: 2
            anchors.centerIn: parent
            anchors.fill: parent
            id:rect
        }
    }

    cursorDelegate: Rectangle {
        width: 2
        color: "#0a9eeb";
        visible: parent.activeFocus && !parent.readOnly && parent.selectionStart === parent.selectionEnd
        id:cursor
        Connections {
            target: cursor.parent
            onCursorPositionChanged: {
                // keep a moving cursor visible
                cursor.opacity = 1
                timer.restart()
            }
        }
        Timer {
            id: timer
            running: cursor.parent.activeFocus && !cursor.parent.readOnly
            repeat: true
            interval: Qt.styleHints.cursorFlashTime / 2
            onTriggered: cursor.opacity = !cursor.opacity ? 1 : 0
            // force the cursor visible when gaining focus
            onRunningChanged: cursor.opacity = 1
        }
    }

    horizontalAlignment:TextField.AlignHCenter
    verticalAlignment:  TextField.AlignVCenter
}