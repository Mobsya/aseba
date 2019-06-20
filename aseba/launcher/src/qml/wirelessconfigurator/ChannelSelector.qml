import QtQuick 2.0

ListView {
    id:view
    property int channel: currentItem ? currentItem.channel : 0
    model: ListModel {
        ListElement {
            channelId: 0
        }
        ListElement {
            channelId: 1
        }
        ListElement {
            channelId: 2
        }
    }
    delegate: ChannelButton {
        channel: channelId
        selected: ListView.isCurrentItem
        onClicked: {
            ListView.view.currentIndex = index;
            view.channel = channel
        }
    }
    orientation: Qt.Horizontal
    spacing: 20

    onChannelChanged: {
        currentIndex = -1
        for (var i = 0; i < count; ++i) {
            if (model.get(i)["channelId"] === channel) {
                 currentIndex = i;
                 break
            }
        }
    }
}
