import QtQuick 2.5
import ".."
import "widgets"

BlockDefinition {
	type: "event"

	defaultParams: [ "DISABLED", "DISABLED"]

	Component {
		id: editorComponent
		Item {
			id: block;

			width: 256
			height: 256

			property var buttons: []

			// shadows
			Rectangle {
				id: leftShadow
				x: 0
				y: 32-100
				width: 128
				height: 200
				color: "transparent"
				clip: true
			}
			Rectangle {
				id: rightShadow
				x: 128
				y: 32-100
				width: 128
				height: 200
				color: "transparent"
				clip: true
			}

			// Thymio body
			ThymioBody {}

			// sensor buttons
			Component.onCompleted: {
				for (var i=0; i<2; ++i) {
					buttons.push(buttonComponent.createObject(block, {
						"x": 100 - 16 + i*56,
						"y": 32 - 16,
						"state": params[i]
					}));
				}
				ledComponent.createObject(block, { "x": 74-12, "y": 32-12, "associatedButton": buttons[0] });
				groundShadowComponent.createObject(leftShadow, { "x": 0, "y": 0, "associatedButton": buttons[0] });
				ledComponent.createObject(block, { "x": 182-12, "y": 32-12, "associatedButton": buttons[1] });
				groundShadowComponent.createObject(rightShadow, { "x": 156-128-100, "y": 0, "associatedButton": buttons[1] });
			}

			function getParams() {
				return buttons.map(function(button) { return button.state; });
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

	Component {
		id: buttonComponent
		InfraredButton {}
	}
	Component {
		id: ledComponent
		InfraredLed {}
	}
	Component {
		id: groundShadowComponent
		ProxGroundShadow {}
	}
}
