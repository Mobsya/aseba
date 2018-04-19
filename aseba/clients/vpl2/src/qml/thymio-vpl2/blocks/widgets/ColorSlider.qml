import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2

Slider {
	id: control

	property color color
	property double bodyValue: (position*32*5.46875+80) / 255.

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
		implicitWidth: horizontal ? 200 : 4
		implicitHeight: horizontal ? 4 : 200
		width: horizontal ? control.availableWidth : implicitWidth
		height: horizontal ? implicitHeight : control.availableHeight
		color: "#b0b0b0"
		scale: horizontal && control.mirrored ? -1 : 1

		readonly property bool horizontal: control.orientation === Qt.Horizontal

		Rectangle {
			x: parent.horizontal ? 0 : (parent.width - width) / 2
			y: parent.horizontal ? (parent.height - height) / 2 : control.visualPosition * parent.height
			width: parent.horizontal ? control.position * parent.width : 4
			height: parent.horizontal ? 4 : control.position * parent.height

			color: control.color
		}
	}

	from: 0
	to: 32
	stepSize: 1
}
