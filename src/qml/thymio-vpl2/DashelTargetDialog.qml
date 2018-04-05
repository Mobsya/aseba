import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3


Popup {
    id: dialog
    x: (parent.width - width) / 2
    y: (parent.height - height) / 2

    height: parent.height * 0.9
    width: parent.width * 0.9
    topMargin: 0

    modal: true
    focus: true
    closePolicy: Popup.OnEscape | Popup.OnPressOutside

    property Aseba aseba

    onVisibleChanged: {

    }
    background: Rectangle {
        color: "#3b3b3b"
        opacity: 0
    }

    contentItem: GridView {

        model: aseba.model
        delegate: Column {
            Image {
                source: "file://home/cor3ntin/dev/mobsya/thymio-vpl2/src/qml/thymio-vpl2/images/icon-thymio.svg"
                anchors.horizontalCenter: parent.horizontalCenter
                width: 100
                fillMode: Image.PreserveAspectFit

            }
            Text {
                text: name;
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#FFFFFF"
            }
        }
    }
}
