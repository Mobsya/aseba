import QtQuick 2.0

Item {
    width: icon.width
    Image {
         id: icon;
         source : "qrc:/assets/battery_mid.svg"
         fillMode: Image.PreserveAspectFit
         clip: true
         height: parent.height
    }
}
