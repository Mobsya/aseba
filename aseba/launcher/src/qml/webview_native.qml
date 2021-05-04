import QtQuick 2.15
import QtWebView 1.15
import QtQuick.Window 2.15
Window {
    id: window
    minimumHeight: 700
    minimumWidth: 1024
    WebView {
        id: webvView
        anchors.fill:parent
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
