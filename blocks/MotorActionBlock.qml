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

			HDPIImage {
				source: "images/motorBackground.svg"
				width: 256 // working around Qt bug with SVG and HiDPI
				height: 256 // working around Qt bug with SVG and HiDPI
			}

			MotorSlider {
				id: leftMotorSlider
				value: params[0]
				x: 52
				y: 70
			}

			MotorSlider {
				id: rightMotorSlider
				value: params[1]
				x: 176
				y: 70
			}

			function getParams() {
				return [leftMotorSlider.value, rightMotorSlider.value];
			}
		}
	}

	function compile(params) {
		return {
			action: "motor.left.target = " + (params[0] | 0) + "\n" + "motor.right.target = " + (params[1] | 0)
		};
	}
}
