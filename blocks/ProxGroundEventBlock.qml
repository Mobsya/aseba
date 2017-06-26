import QtQuick 2.7
import ".."
import "widgets"

BlockDefinition {
	type: "event"
	category: "ground"

	defaultParams: [ "DISABLED", "DISABLED"]

	Component {
		id: editorComponent
		Item {
			id: block;

			width: 256
			height: 256

			// shadows
			Rectangle {
				id: leftShadow
				x: 0
				y: 32-100
				width: 128
				height: 200
				color: "transparent"
				clip: true
				ProxGroundShadow {
					x: 0
					y: 0
					associatedButton: leftButton
				}
			}
			Rectangle {
				id: rightShadow
				x: 128
				y: 32-100
				width: 128
				height: 200
				color: "transparent"
				clip: true
				ProxGroundShadow {
					x: 156-128-100
					y: 0
					associatedButton: rightButton
				}
			}

			// Thymio body
			ThymioBody {}

			// sensor buttons
			InfraredButton {
				id: leftButton
				x: 100 - 16
				y: 32 - 16
				state: params[0]
				InfraredLed {
					x: -22
					y: 4
					associatedButton: leftButton
				}
			}
			InfraredButton {
				id: rightButton
				x: 100 - 16 + 56
				y: 32 - 16
				state: params[1]
				InfraredLed {
					x: 30
					y: 4
					associatedButton: rightButton
				}
			}

			function getParams() {
				return [leftButton.state, rightButton.state];
			}
		}
	}

	editor: Component {
		Loader {
			sourceComponent: editorComponent
			scale: 0.9
			property var params: defaultParams
			function getParams() { return item.getParams(); }
		}
	}

	miniature: Component {
		Loader {
			sourceComponent: editorComponent
			scale: 0.6
			property var params: defaultParams
		}
	}

	function compile(params) {
		return {
			event: "prox",
			condition: params.reduce(function(source, param, index) {
				if (param === "DISABLED") {
					return source;
				}
				source += " and prox.ground.delta[" + index + "] "
				if (param === "CLOSE") {
					source += "> 450";
				} else {
					source += "< 400";
				}
				return source;
			}, "0 == 0"),
		};
	}
}
