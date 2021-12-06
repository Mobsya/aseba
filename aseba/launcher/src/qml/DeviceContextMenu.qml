import QtQuick 2.12
import QtQuick.Controls 2.4
import QtGraphicalEffects 1.0

Item {
    id: component
    property Menu menu : null
    signal open()

    function popup() {
        if(!menu)
            menu = builder.createObject(component)
        menu.popup(component.parent)
        open()
    }

    Component {
        id:builder
        Menu {
            property var device
            id: menu

            onClosed: {
                menu.destroy()
                component.menu = null
            }

            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

            Item {
                height: 32
                width: bg.width - 2 * 10
                anchors.horizontalCenter: parent.horizontalCenter
                Text {
                    text: device.name
                    font.family: "Roboto"
                    font.pointSize: 9
                    color : "white"
                    verticalAlignment: Text.AlignVCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MenuSeparator {
                contentItem: Rectangle {
                    width: menu.width
                    implicitHeight: 1
                    color: "#2e2e2e"
                }
            }

            topPadding: 10
            bottomPadding: 10


            delegate: MenuItem {
                id: menuItem
                width: bg.width
                height: 32

                onParentChanged: {
                    if(parent)
                        anchors.horizontalCenter = parent.horizontalCenter
                }

                contentItem: Item {
                    width: bg.width - 2 *10
                    anchors.horizontalCenter: parent.horizontalCenter
                    height: parent.height

                    Text {
                        text: menuItem.text
                        font.family: "Roboto"
                        font.bold: true
                        font.pointSize: 9
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                background: Rectangle {
                    anchors.fill: parent
                    opacity: 1
                    color: menuItem.highlighted ? Style.verydark : "transparent"
                }
            }

            background: Item {
                id: wholebg
                implicitWidth: 200

                Item {
                    id: shadowContainer
                    anchors.fill: parent
                    visible: false

                    Rectangle {
                       id:bg
                       color: Style.dark
                       radius: 5
                       antialiasing: true
                       anchors.centerIn: parent
                       width: parent.width - 2 * bgShadow.radius
                       height: parent.height - 2 * bgShadow.radius
                    }
                }

                DropShadow {
                   id: bgShadow
                   anchors.fill: source
                   horizontalOffset: 0
                   verticalOffset: 0
                   radius: 6
                   samples: 17
                   color: Qt.rgba(0, 0, 0, 0.8)
                   source: shadowContainer
                   smooth: true

                }
            }
        }
    }
    InverseMouseArea {
        enabled: menu && menu.visible
        onPressed:  {
            if(menu) {
                menu.dismiss()
            }
        }
        anchors.fill: parent
    }
}