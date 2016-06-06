import QtQuick 2.0
import QtQuick.Controls 2.0
//import QtQuick.Controls 1.4
//import QtQuick.Controls.Styles 1.4

Slider {
	width: 28
	height: 116
	orientation: Qt.Vertical
	snapMode: Slider.SnapAlways

	//onMousePressEvent: positionAt()

//	style: SliderStyle {
//		groove: Item {}
//		handle: Rectangle {
//			anchors.centerIn: parent
//			color: "#ec1e24"
//			implicitWidth: 16
//			implicitHeight: 28
//		}
//	}

	background: null

	from: -500
	to: 500
	stepSize: 50
}
