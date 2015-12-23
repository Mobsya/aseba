import QtQuick 2.5
import ".."
import "widgets"

BlockDefinition {
	type: "action"

	defaultParams: [ 0, 0, 0 ]

	editor: Component {
		Item {
			width: 256
			height: 256
			property var params: defaultParams

			Rectangle {
				width: 236
				height: 236
				radius: 118
				color: Qt.rgba(red.bodyValue(), green.bodyValue(), blue.bodyValue(), 1.0)
				anchors.centerIn: parent
			}

			ColorSlider {
				id: red
				color: "red"
				x: 38
				y: 52
				value: params[0]
			}
			ColorSlider {
				id: green
				color: "green"
				x: 38
				y: 52+54
				value: params[1]
			}
			ColorSlider {
				id: blue
				color: "blue"
				x: 38
				y: 52+54*2
				value: params[2]
			}

			function getParams() {
				return [red.value, green.value, blue.value];
			}
		}
	}

	function compile(params) {
		return {
			action: "call leds.top(" + params[0] + ", " + params[1] + ", " + params[2] + ")"
		};
	}
}
