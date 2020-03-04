import QtQuick 2.0
import QtWebView 1.1
import QtQuick.Window 2.12
import QtQuick.Controls 1.1
Window {
    id: window
    minimumHeight: 700
    minimumWidth: 1024

    TextField {
        id: urlField
        anchors.right:parent.right
        anchors.left:parent.left
        anchors.margins: 4
        height: font.pixelSize * 2.5
        readOnly: true
        text: appUrl
    }
    WebView {
        id: webvView
        anchors.top:urlField.bottom
        anchors.right:parent.right
        anchors.bottom:parent.bottom
        anchors.left:parent.left
        url: appUrl
        onTitleChanged: {
            window.title = title
        }
    }
    Component.onCompleted: {
        window.show();
    }
    onClosing: {
        webvView.loadHtml("<html></html>")
        //webvView.stop()
        Qt.quit();
    }
}