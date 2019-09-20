import QtQuick 2.0
import QtWebView 1.1
import QtQuick.Window 2.12
Window {
    id: window
    width: ScreenInfo.width
    height: ScreenInfo.height
    Rectangle
    {
        anchors.fill:parent
        Rectangle{
            id:topmargin
            width: parent.width
            height: 30
            color : Style.main_bg_color
        }

        SvgButton {
            source: "qrc:/assets/launcher-icon-close.svg"
            height: 30
            width: 30
            id:closebutton
            anchors.right : parent.right
            anchors.top: parent.top
            onClicked: {
              close();
            }
        }

        WebView {
            id: webvView
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: topmargin.bottom
            url: appUrl
            onTitleChanged: {
                window.title = title
            }
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
