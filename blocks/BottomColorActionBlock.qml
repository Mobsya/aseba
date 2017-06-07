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
				bottomColor: Qt.rgba(red.bodyValue, green.bodyValue, blue.bodyValue, Math.max(red.bodyValue, green.bodyValue, blue.bodyValue))
			}

			ColorSlider {
				id: red
				color: "#ff0000"
				x: 38
				y: 136
				value: params[0]
			}
			ColorSlider {
				id: green
				color: "#00ff00"
				x: 38
				y: 136+40
				value: params[1]
			}
			ColorSlider {
				id: blue
				color: "#0000ff"
				x: 38
				y: 136+40*2
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
				y: -10
				bottomColor: Qt.rgba(paramToColor(params[0]), paramToColor(params[1]), paramToColor(params[2]), Math.max(paramToColor(params[0]), paramToColor(params[1]), paramToColor(params[2])))
			}

			Item {
				property color color: Qt.rgba(params[0] / 32., params[1] / 32., params[2] / 32.)
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.bottom: parent.bottom
				anchors.bottomMargin: 45
				width: 130
				height: 24
				Rectangle {
					anchors.left: parent.left
					anchors.verticalCenter: parent.verticalCenter
					width: 100
					height: 12
					color: parent.color
				}
				Rectangle {
					anchors.right: parent.right
					anchors.verticalCenter: parent.verticalCenter
					width: 30
					height: 6
					color: "white"
				}
				Rectangle {
					anchors.left: parent.left
					anchors.leftMargin: 100 - width/2
					anchors.verticalCenter: parent.verticalCenter
					width: 24
					height: 24
					radius: 12
					color: parent.color
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
