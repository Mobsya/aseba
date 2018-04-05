import QtQuick 2.7
import QtQuick.Controls 2.2

Slider {
	width: 28
	height: 116
	orientation: Qt.Vertical
	snapMode: Slider.SnapAlways

	background: null

	from: -500
	to: 500
	stepSize: 50
}
