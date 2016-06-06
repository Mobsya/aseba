import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import QtQuick.Controls.Material.impl 2.0

Slider {
	id: control
	property color color

	width: 180
	snapMode: Slider.SnapAlways

	handle: ColoredSliderHandle {
		x: control.leftPadding + (horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
		y: control.topPadding + (horizontal ? (control.availableHeight - height) / 2 : control.visualPosition * (control.availableHeight - height))
		value: control.value
		handleHasFocus: control.visualFocus
		handlePressed: control.pressed
		color: control.color
	}

	background: Rectangle {
		x: control.leftPadding + (horizontal ? 0 : (control.availableWidth - width) / 2)
		y: control.topPadding + (horizontal ? (control.availableHeight - height) / 2 : 0)
		implicitWidth: horizontal ? 200 : 1
		implicitHeight: horizontal ? 1 : 200
		width: horizontal ? control.availableWidth : implicitWidth
		height: horizontal ? implicitHeight : control.availableHeight
		color: control.Material.primaryTextColor
		scale: horizontal && control.mirrored ? -1 : 1

		readonly property bool horizontal: control.orientation === Qt.Horizontal

		Rectangle {
			x: parent.horizontal ? 0 : (parent.width - width) / 2
			y: parent.horizontal ? (parent.height - height) / 2 : control.visualPosition * parent.height
			width: parent.horizontal ? control.position * parent.width : 3
			height: parent.horizontal ? 3 : control.position * parent.height

			color: control.color
		}
	}

	from: 0
	to: 32
	stepSize: 1

	function bodyValue() {
		return (value*5.46875+80) / 255.;
	}
}
