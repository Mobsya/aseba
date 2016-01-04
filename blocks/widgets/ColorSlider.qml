import QtQuick 2.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Slider {
	id: slider
	property color color

	width: 180
	height: 40

	style: SliderStyle {
		groove: Rectangle {
			implicitWidth: 180
			implicitHeight: 28
			color: slider.color
		}
		handle: Rectangle {
			anchors.centerIn: parent
			color: "white"
			implicitWidth: 20
			implicitHeight: 40
		}
	}

	minimumValue: 0
	maximumValue: 32
	stepSize: 1

	function bodyValue() {
		return (value*5.46875+80) / 255.;
	}
}
