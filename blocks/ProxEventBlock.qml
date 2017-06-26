import QtQuick 2.7
import ".."
import "widgets"

BlockDefinition {
	type: "event"
	category: "prox"

	defaultParams: [ "DISABLED", "DISABLED", "DISABLED", "DISABLED", "DISABLED", "DISABLED", "DISABLED" ]

	Component {
		id: editorComponent
		Item {
			id: block;

			width: 256
			height: 256

			// Thymio body
			ThymioBody {}

			Repeater {
				id: frontSensors
				model: 5
				InfraredButton {
					id: button
					property int offset: 2.0 - index
					x: 128 - 16 - 150*Math.sin(0.34906585*offset)
					y: 172 - 16 - 150*Math.cos(0.34906585*offset)
					rotation: -20*offset
					state: params[index]
					InfraredLed {
						visible: index <= 2
						x: -20
						y: 8
						associatedButton: button
					}
					InfraredLed {
						visible: index >= 2
						x: 28
						y: 8
						associatedButton: button
					}
				}
			}

			Repeater {
				id: backSensors
				model: 2
				InfraredButton {
					id: button
					x: 64 - 16 + index*128
					y: 234 - 16
					state: params[index+5]
					InfraredLed {
						x: -20 + index*48
						y: 4
						associatedButton: button
					}
				}
			}

			function getParams() {
				return [
					frontSensors.itemAt(0).state,
					frontSensors.itemAt(1).state,
					frontSensors.itemAt(2).state,
					frontSensors.itemAt(3).state,
					frontSensors.itemAt(4).state,
					backSensors.itemAt(0).state,
					backSensors.itemAt(1).state,
				];
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
				source += " and prox.horizontal[" + index + "] "
				if (param === "CLOSE") {
					source += "> 2000";
				} else {
					source += "< 1000";
				}
				return source;
			}, "0 == 0"),
		};
	}
}
