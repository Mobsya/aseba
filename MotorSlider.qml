import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Slider {
	width: 48
	height: 226
	orientation: Qt.Vertical

	style: SliderStyle {
		groove: Item {}
		handle: Rectangle {
			anchors.centerIn: parent
			color: "transparent"
			border.color: "black"
			border.width: 4
			implicitWidth: 36
			implicitHeight: 36
		}
	}

	minimumValue: -10
	maximumValue: 10
	stepSize: 1
}
