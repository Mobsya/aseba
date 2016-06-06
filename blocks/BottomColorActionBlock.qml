import QtQuick 2.5
import ".."
import "widgets"

BlockDefinition {
	type: "action"

	defaultParams: [ 32, 0, 16 ]

	editor: Component {
		Item {
			width: 256
			height: 256
			property var params: defaultParams

			ThymioFront {
				y: -80
				bottomColor: Qt.rgba(red.bodyValue(), green.bodyValue(), blue.bodyValue(), Math.max(red.bodyValue(), green.bodyValue(), blue.bodyValue()))
			}

			ColorSlider {
				id: red
				color: "#ff0000"
				x: 38
				y: 106
				value: params[0]
			}
			ColorSlider {
				id: green
				color: "#00ff00"
				x: 38
				y: 106+50
				value: params[1]
			}
			ColorSlider {
				id: blue
				color: "#0000ff"
				x: 38
				y: 106+50*2
				value: params[2]
			}

			function getParams() {
				return [red.value, green.value, blue.value];
			}
		}
	}

	miniature: Component {
		Item {
			width: 256
			height: 256
			property var params: defaultParams

			function paramToColor(param) {
				return (param*5.46875+80) / 255.;
			}

			ThymioFront {
				bottomColor: Qt.rgba(paramToColor(params[0]), paramToColor(params[1]), paramToColor(params[2]), Math.max(paramToColor(params[0]), paramToColor(params[1]), paramToColor(params[2])))
			}
		}
	}

	function compile(params) {
		return {
			action: "call leds.bottom.left(" + params[0] + ", " + params[1] + ", " + params[2] + ")\n" +
					"call leds.bottom.right(" + params[0] + ", " + params[1] + ", " + params[2] + ")"
		};
	}
}
