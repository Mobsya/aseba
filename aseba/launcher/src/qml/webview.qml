import QtQuick 2.15
import QtWebEngine 1.10
import QtQuick.Window 2.15
Window {
    id: window
    minimumHeight: 700
    minimumWidth: 1024
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
            featurePermissionRequested.connect(onPermissionPequested)
        }
        function download(request) {
            request.path = Utils.getDownloadPath(request.path);
            if(request.path !== "")
                request.accept()
        }
        function onPermissionPequested(url, request) {
           if(url.toString().startsWith("file://"))
               grantFeaturePermission(url, request, true)
        }
    }
    Component.onCompleted: {
        window.show();
    }
    onClosing: {
        Qt.quit();
    }
}
