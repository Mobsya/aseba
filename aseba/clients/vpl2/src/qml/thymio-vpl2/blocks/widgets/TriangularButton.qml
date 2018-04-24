import QtQuick 2.7
import "../.."

HDPIImage {
	state: "DISABLED"
	source: state === "DISABLED" ? "TriangularButtonOff.svg" : "TriangularButtonOn.svg"
	width: 58 // working around Qt bug with SVG and HiDPI
	height: 58 // working around Qt bug with SVG and HiDPI

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
