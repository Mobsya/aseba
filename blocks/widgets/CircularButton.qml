import QtQuick 2.0
import "../.."

HDPIImage {
	state: "DISABLED"
	source: state === "DISABLED" ? "images/circularButtonOff.svg" : "images/circularButtonOn.svg"
	width: 64 // working around Qt bug with SVG and HiDPI
	height: 64 // working around Qt bug with SVG and HiDPI

	MouseArea {
		anchors.fill: parent
		onClicked: {
			if (parent.state === "DISABLED")
				parent.state = "ON";
			else
				parent.state = "DISABLED";
		}
	}
}
