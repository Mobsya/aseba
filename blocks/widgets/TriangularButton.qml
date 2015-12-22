import QtQuick 2.0

Image {
	state: "DISABLED"
	source: state === "DISABLED" ? "images/triangularButtonOff.svg" : "images/triangularButtonOn.svg"

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
