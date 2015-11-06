import QtQuick 2.5


Item {
	id: block;
	width: 256
	height: 256

	Image {
		source: "images/eventBg.svg"

	}

	Image {
		source: "images/eventCenter.svg"
	}

	Image {
		id: linkingPath

		source: "images/lineDotted.png"
		visible: false
		fillMode: Image.TileHorizontally

		transform: Rotation {
			id: linkingPathRotation
			origin { x: 0; y: 0 }
			angle: 0
		}
	}

	// TODO: move somewhere else
	function toDegrees (angle) {
		return angle * (180 / Math.PI);
	}

	// link
	MouseArea {
		id: linkArea
		anchors.fill: parent
		scrollGestureEnabled: false  // 2-finger-flick gesture should pass through to the Flickable

		onPressed: {
			// within inner radius
			var dx = mouse.x - 128;
			var dy = mouse.y - 128;
			mouse.accepted = dx*dx + dy*dy < 128*128;
			if (mouse.accepted) {
				linkingPath.x = mouse.x;
				linkingPath.y = mouse.y;
				linkingPath.visible = true;
			}
		}

		onPositionChanged: {
			var dx = mouse.x - linkingPath.x;
			var dy = mouse.y - linkingPath.y;
			linkingPath.width = Math.sqrt(dx*dx + dy*dy);
			linkingPathRotation.angle = toDegrees(Math.atan2(dy, dx));
		}

		onReleased: {
			linkingPath.visible = false;
			// TODO: handle connections
		}
	}

	// drag
	MouseArea {
		id: dragArea
		anchors.fill: parent
		drag.target: block
		scrollGestureEnabled: false  // 2-finger-flick gesture should pass through to the Flickable

		onPressed: {
			// within inner radius
			mouse.accepted = function () {
				var dx = mouse.x - 128;
				var dy = mouse.y - 128;
				return dx*dx+dy*dy < 99*99;
			} ();
			if (block.z < scene.highestZ) {
				block.z = ++scene.highestZ;
			}
		}

		onReleased: {
			scene.setContentSize();
		}
	}
}

