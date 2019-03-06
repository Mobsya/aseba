import QtQuick 2.0
import QtWebEngine 1.7
import QtQuick.Window 2.3
Window {
    minimumHeight: 700
    minimumWidth : 1024
    id: window
    WebEngineView {
        anchors.fill: parent
        url: appUrl
        onTitleChanged: {
            window.title = title
        }
        onNewViewRequested: {
            Qt.openUrlExternally(request.requestedUrl)
        }
        Component.onCompleted: {
            profile.downloadRequested.connect(download)
        }
        function download(request) {
            request.path = Utils.getDownloadPath(request.path);
            if(request.path !== "")
                request.accept()
        }
    }
}