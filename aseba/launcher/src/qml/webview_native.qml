import QtQuick 2.0
import QtWebView 1.1
WebView {
    id: webvView
    anchors.fill: parent
    url: appUrl
    onTitleChanged: {
        window.title = title
    }
    //Unload the webpage upon closing to avoid a crash on OSX
    Component.onCompleted: {
        window.closing.connect(function() {
            webvView.stop()
            webvView.url = ""
        });
    }
}
