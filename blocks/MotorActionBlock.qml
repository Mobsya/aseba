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
			property real animationValue: 0

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
				onValueChanged: robotAnimation.restart()
			}

			MotorSlider {
				id: rightMotorSlider
				value: params[1]
				x: 176
				y: 70
				onValueChanged: robotAnimation.restart()
			}

			NumberAnimation on animationValue {
				id: robotAnimation
				from: 0
				to: 150
				duration: 2000
			}

			Item {
				scale: 0.2
				property real wheelsSum: leftMotorSlider.value + rightMotorSlider.value
				property real wheelsDiff: leftMotorSlider.value - rightMotorSlider.value
				property real rotationAngle: wheelsDiff * animationValue * 0.0009
				//transform: Rotation { origin.x: 128; origin.y: 128; angle: miniThymio.rotationAngle }
				rotation: rotationAngle
				width: miniThymio.width
				height: miniThymio.height
				x: wheelsDiff === 0 ? 0 : (23.5 * wheelsSum / wheelsDiff) * (1. - Math.cos(-rotation * Math.PI / 180))
				y: wheelsDiff === 0 ? (-wheelsSum * animationValue * 0.00036) : (23.5 * wheelsSum / wheelsDiff) * Math.sin(-rotation * Math.PI / 180)
				Image {
					id: miniThymio
					source: "widgets/images/thymioBody.svg"
					x: 0
					y: -55
				}
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
