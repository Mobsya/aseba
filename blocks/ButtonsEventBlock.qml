import QtQuick 2.7
import ".."
import "widgets"

BlockDefinition {
	type: "event"
	category: "buttons"

	defaultParams: [ "DISABLED", "DISABLED", "DISABLED", "DISABLED", "DISABLED" ]

	editor: Component {
		Item {
			id: block;

			width: 256
			height: 256
			property var params: defaultParams

			// top
			TriangularButton {
				id: button0
				anchors.horizontalCenter: parent.horizontalCenter
				y: 36
				state: params[0]
			}
			// left
			TriangularButton {
				id: button1
				anchors.verticalCenter: parent.verticalCenter
				x: 36
				rotation: -90
				state: params[1]
			}
			// bottom
			TriangularButton {
				id: button2
				anchors.horizontalCenter: parent.horizontalCenter
				y:162
				rotation: 180
				state: params[2]
			}
			// right
			TriangularButton {
				id: button3
				anchors.verticalCenter: parent.verticalCenter
				x:162
				rotation: 90
				state: params[3]
			}
			// center
			CircularButton {
				id: button4
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.verticalCenter: parent.verticalCenter
				state: params[4]
			}

			function getParams() {
				return [button0.state, button1.state, button2.state, button3.state, button4.state];
			}
		}
	}

	function compile(params) {
		var condition = params.reduce(function(source, param, index) {
			if (param === "DISABLED") {
				return source;
			}

			var names = ["forward", "left", "backward", "right", "center"];
			var condition = "button." + names[index] + " == 1";

			if (source === "") {
				return condition;
			}
			return source + " and " + condition;
		}, "");

		if (condition === "") {
			throw qsTr("No button selected");
		}

		return {
			event: "buttons",
			condition: condition,
		};
	}
}
