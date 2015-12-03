import QtQuick 2.5

// this block is both the miniature and the editor
Item {
	width: 256
	height: 256

	property var params: [ 0, 0 ]

	// Thymio body
	//ThymioBody {}

	Image {
		source: "images/vpl_background_motor.svgz"
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

	// in editor mode
	function getType() {
		return "action";
	}

	// in editor mode
	function getName() {
		return "motor";
	}

	// in editor mode
	function getParams() {
		return [leftMotorSlider.value, rightMotorSlider.value];
	}

	// in editor mode
	function getMiniature() {
		var motorActionBlockComponent = Qt.createComponent("MotorActionBlock.qml");
		var miniatureBlock = motorActionBlockComponent.createObject(null, {
			"params": getParams(),
			"enabled": false
		});
		miniatureBlock.scale = 0.5;
		return miniatureBlock;
	}

	// in miniature mode
	function createEditor(parentItem) {
		var motorActionBlockComponent = Qt.createComponent("MotorActionBlock.qml");
		return motorActionBlockComponent.createObject(parentItem, {
			"params": getParams()
		});
	}
}
