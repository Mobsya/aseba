import QtQuick 2.5

Rectangle {
	id: block;

	// event block background
	width: 256
	height: 256
	color: "#f6871f"
	antialiasing: true

	// Thymio body
	Rectangle {
		width: 248
		height: 176
		x: 4
		y: 76
		radius: 5
		color: "white"
	}
	Rectangle {
		width: 248
		height: 144
		x: 4
		y: 4
		radius: width
	}

	// drag
	MouseArea {
		id: dragArea
		//hoverEnabled: true
		anchors.fill: parent
		drag.target: parent
		scrollGestureEnabled: false  // 2-finger-flick gesture should pass through to the Flickable

		onPressed: {
			if (block.z < scene.highestZ) {
				block.z = ++scene.highestZ;
			}
		}

		onReleased: {
			scene.setContentSize();
		}
	}

	// sensor buttons
	Component.onCompleted: {

		// factory for buttons
		var buttonComponent = Qt.createComponent("InfraredButton.qml");
		var ledComponent = Qt.createComponent("InfraredLed.qml");
		var offset;
		var buttons = [];
		var led;

		// front sensors
		for (var i=0; i<5; ++i) {
			offset = 2.0 - i;
			buttons.push(buttonComponent.createObject(block, {
				"x": 128 - 16 - 150*Math.sin(0.34906585*offset),
				"y": 175 - 16 - 150*Math.cos(0.34906585*offset),
				"rotation": -20*offset
			}));
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
				"y": 234 - 16
			}));
		}
		ledComponent.createObject(block, { "x": 40-12, "y": 234-12, "associatedButton": buttons[5] });
		ledComponent.createObject(block, { "x": 216-12, "y": 234-12, "associatedButton": buttons[6] });
	}
}

