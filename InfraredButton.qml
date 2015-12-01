import QtQuick 2.0

Rectangle {
	id: buttonRect

	width: 32
	height: 32
	transformOrigin: Item.Center
	color: "#dcdcdc"
	border.color: "lightGray"
	border.width: 5
	state: "DISABLED"

	MouseArea {
		anchors.fill: parent

		onClicked: {
			if (buttonRect.state == "DISABLED")
				buttonRect.state = "CLOSE";
			else if (buttonRect.state == "CLOSE")
				buttonRect.state = "FAR";
			else
				buttonRect.state = "DISABLED";
		}
	}

	states: [
		State {
			name: "CLOSE"
			PropertyChanges { target: buttonRect; color: "white"; border.color: "red"; }
		},
		State {
			name: "FAR"
			PropertyChanges { target: buttonRect; color: "black"; border.color: "black"; }
		}
	]

	transitions:
		Transition {
			to: "*"
			ColorAnimation { target: buttonRect; duration: 50}
		}
}

