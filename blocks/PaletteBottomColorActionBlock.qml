import QtQuick 2.7
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
				y: -60
				bottomColor: palette.bodyColor
			}

			ColorPalette {
				id: palette
				color: Qt.rgba(params[0] / 32., params[1] / 32., params[2] / 32.)
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.bottom: parent.bottom
				anchors.bottomMargin: 20
			}

			function getParams() {
				return [Math.round(palette.color.r * 32.), Math.round(palette.color.g * 32.), Math.round(palette.color.b * 32)];
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
				y: -10
				bottomColor: Qt.rgba(paramToColor(params[0]), paramToColor(params[1]), paramToColor(params[2]), Math.max(paramToColor(params[0]), paramToColor(params[1]), paramToColor(params[2])))
			}

			Row {
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.bottom: parent.bottom
				anchors.bottomMargin: 45
				Rectangle {
					width: 20
					height: 20
					color: "#ffbf00"
				}
				Rectangle {
					width: 20
					height: 20
					color: "#00ff40"
				}
				Rectangle {
					width: 20
					height: 20
					color: "#0040ff"
				}
				Rectangle {
					width: 20
					height: 20
					color: "#ff00bf"
				}
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
