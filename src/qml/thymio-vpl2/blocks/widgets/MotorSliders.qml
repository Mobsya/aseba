import QtQuick 2.7
import "../.."

Item {
	width: 256
	height: 256
	property var params
	property bool miniature
	property real animationValue: 0

	HDPIImage {
		source: miniature ? "MotorSlidersMini.svg" : "MotorSlidersEdit.svg"
		width: 256 // working around Qt bug with SVG and HiDPI
		height: 256 // working around Qt bug with SVG and HiDPI
	}

	MotorSlider {
		id: leftMotorSlider
		value: params[0]
		x: miniature ? 52 : 22
		y: miniature ? 70 : 40
		height: miniature ? 116 : 176
		onValueChanged: robotAnimation.restart()
	}

	MotorSlider {
		id: rightMotorSlider
		value: params[1]
		x: miniature ? 176 : 206
		y: miniature ? 70 : 40
		height: miniature ? 116 : 176
		onValueChanged: robotAnimation.restart()
	}

	NumberAnimation on animationValue {
		id: robotAnimation
		from: 0
		to: miniature ? 150 : 180
		duration: miniature ? 2000 : 2400
	}

	// internal properties for computing animation
	property real wheelsSum: leftMotorSlider.value + rightMotorSlider.value
	property real wheelsDiff: leftMotorSlider.value - rightMotorSlider.value
	property real factorRotation: wheelsDiff * 0.0009
	property real factorWhenRot: miniature ? 23.5 : 28.2
	property real factorWhenStraight: miniature ? 0.00036 : 0.000432
	property real factorX: (factorWhenRot * wheelsSum / wheelsDiff)
	property real factorYTurn: (factorWhenRot * wheelsSum / wheelsDiff)
	property real factorYStraight: -wheelsSum * factorWhenStraight

	/*
	// this shows a trail, but it is not very aesthetic
	Repeater {
		model: 7
		Rectangle {
			scale: miniature ? 0.18 : 0.22
			property real curAnimationValue: index * 30
			property real curRotationAngle: factorRotation * curAnimationValue
			visible: !miniature && animationValue > curAnimationValue
			width: 20
			height: 20
			color: "#808080"
			radius: 10
			x: wheelsDiff === 0 ? 118 : 118 + factorX * (1. - Math.cos(-curRotationAngle * Math.PI / 180))
			y: wheelsDiff === 0 ? 118 + factorYStraight * curAnimationValue : 118 + factorYTurn * Math.sin(-curRotationAngle * Math.PI / 180)
		}
	}*/

	Item {
		scale: miniature ? 0.18 : 0.22

		property real rotationAngle: factorRotation * animationValue
		//transform: Rotation { origin.x: 128; origin.y: 128; angle: miniThymio.rotationAngle }
		rotation: rotationAngle
		width: miniThymio.width
		height: miniThymio.height
		x: wheelsDiff === 0 ? 0 : factorX * (1. - Math.cos(-rotation * Math.PI / 180))
		y: wheelsDiff === 0 ? factorYStraight * animationValue : factorYTurn * Math.sin(-rotation * Math.PI / 180)
		Image {
			id: miniThymio
			source: "ThymioBody.svg"
			x: 0
			y: -55
		}
	}

	function getParams() {
		return [leftMotorSlider.value, rightMotorSlider.value];
	}
}
