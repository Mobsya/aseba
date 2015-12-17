import QtQuick 2.5
import ".."
import "widgets"

BlockDefinition {
	type: "action"
	defaultParams: [ 0, 0 ]
	editor: Component {
		Item {
			width: 256
			height: 256
			property var params: defaultParams

			// Thymio body
			//ThymioBody {}

			Image {
				source: "images/motorBackground.svgz"
			}

			MotorSlider {
				id: leftMotorSlider
				value: params[0]
				x: 10
				y: 15
			}

			MotorSlider {
				id: rightMotorSlider
				value: params[1]
				x: 198
				y: 15
			}

			function getParams() {
				return [leftMotorSlider.value, rightMotorSlider.value];
			}
		}
	}
	function compile(params) {
		return {
			action: "motor.left.target = " + params[0] + "\n" + "motor.right.target = " + params[1]
		};
	}
}
