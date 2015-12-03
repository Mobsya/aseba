import QtQuick 2.5

BlockDefinition {
	type: "event"
	defaultParams: [ "DISABLED", "DISABLED", "DISABLED", "DISABLED", "DISABLED", "DISABLED", "DISABLED" ]
	editor: Component {
		Item {
			id: block;

			width: 256
			height: 256
			property var params: defaultParams

			property var buttons: []

			// Thymio body
			ThymioBody {}

			// sensor buttons
			Component.onCompleted: {
				// factory for buttons
				var buttonComponent = Qt.createComponent("InfraredButton.qml");
				var ledComponent = Qt.createComponent("InfraredLed.qml");
				var offset;
				var led;

				// front sensors
				for (var i=0; i<5; ++i) {
					offset = 2.0 - i;
					var button = buttonComponent.createObject(block, {
																  "x": 128 - 16 - 150*Math.sin(0.34906585*offset),
																  "y": 175 - 16 - 150*Math.cos(0.34906585*offset),
																  "rotation": -20*offset,
																  "state": params[i]
															  });
					buttons.push(button);
				}
				ledComponent.createObject(block, { "x": 15-12, "y": 78-12, "associatedButton": buttons[0] });
				ledComponent.createObject(block, { "x": 54-12, "y": 43-12, "associatedButton": buttons[1] });
				ledComponent.createObject(block, { "x": 104-12, "y": 26-12, "associatedButton": buttons[2] });
				ledComponent.createObject(block, { "x": 152-12, "y": 26-12, "associatedButton": buttons[2] });
				ledComponent.createObject(block, { "x": 202-12, "y": 43-12, "associatedButton": buttons[3] });
				ledComponent.createObject(block, { "x": 241-12, "y": 78-12, "associatedButton": buttons[4] });

				// back sensors
				for (var i=0; i<2; ++i) {
					buttons.push(buttonComponent.createObject(block, {
						"x": 64 - 16 + i*128,
						"y": 234 - 16,
						"state": params[i+5]
					}));
				}
				ledComponent.createObject(block, { "x": 40-12, "y": 234-12, "associatedButton": buttons[5] });
				ledComponent.createObject(block, { "x": 216-12, "y": 234-12, "associatedButton": buttons[6] });
			}

			function getParams() {
				return buttons.map(function(button) { return button.state; });
			}
		}
	}
}
