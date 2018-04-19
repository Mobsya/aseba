import QtQuick 2.7
import "../.."

Image {
	property int majorValue: 0
	property real minorValue: 1

	source: majorValue === 0 ? "TimerGlass/shortStroke.svg" : (majorValue === 1 ? "TimerGlass/middleStroke.svg" : "TimerGlass/longStroke.svg")
	width: 256 // working around Qt bug with SVG and HiDPI
	height: 256 // working around Qt bug with SVG and HiDPI

	HDPIImage {
		anchors.horizontalCenter: parent.horizontalCenter
		y: 99
		source: majorValue === 0 ? "TimerGlass/shortFill.svg" : (majorValue === 1 ? "TimerGlass/middleFill.svg" : "TimerGlass/longFill.svg")
		width: majorValue === 0 ? 38 : (majorValue === 1 ? 60 : 80) // working around Qt bug with SVG and HiDPI
		height: 25.4 // working around Qt bug with SVG and HiDPI
		visible: minorValue > 0
	}

	Rectangle {
		anchors.horizontalCenter: parent.horizontalCenter
		width: majorValue === 0 ? 38 : (majorValue === 1 ? 60 : 80)
		height: 6.3 * (minorValue-1)
		y: 99 - height
		color: "#231f20"
		visible: minorValue > 1
	}
}
