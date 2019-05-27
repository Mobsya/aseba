import QtQuick 2.0

ListView {
    property int channel: currentItem ? currentItem.channel : 0
    model: ListModel {
        ListElement {
            channelId: 1
        }
        ListElement {
            channelId: 2
        }
        ListElement {
            channelId: 3
        }
    }
    delegate: ChannelButton {
        channel: channelId
        selected: ListView.isCurrentItem
        onClicked: {
            ListView.view.currentIndex = index;
        }
    }
    orientation: Qt.Horizontal
    spacing: 20
}
