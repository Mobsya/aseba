import QtQuick 2.5


Item {
	id: block;
	width: 256
	height: 256
	z: 1

	property string blockName: "genericEvent"

	Image {
		source: "images/eventBg.svg"

	}

	Image {
		source: "images/eventCenter.svg"
	}

	LinkingPath {
		id: linkingPath
		visible: false
	}

	// TODO: move somewhere else
	function toDegrees (angle) {
		return angle * (180 / Math.PI);
	}
	function toRadians (angle) {
		return angle * Math.PI / 180;
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
			// TODO: add another test for small handle, once we are sure we want to keep it
			if (mouse.accepted) {
				linkingPath.x = mouse.x;
				linkingPath.y = mouse.y;
				linkingPath.width = 1;
				linkingPath.visible = true;
			}
		}

		onPositionChanged: {
			var dx = mouse.x - linkingPath.x;
			var dy = mouse.y - linkingPath.y;
			linkingPath.width = Math.sqrt(dx*dx + dy*dy);
			linkingPath.rotationAngle = toDegrees(Math.atan2(dy, dx));
		}

		onReleased: {
			linkingPath.visible = false;

			var scenePos = mapToItem(scene, mouse.x, mouse.y);
			var destBlock = scene.contentItem.childAt(scenePos.x, scenePos.y);

			if (destBlock && destBlock.blockName) {
				// create connections
				var path_start = mapToItem(scene, linkingPath.x, linkingPath.y)
				var blockLinkComponent = Qt.createComponent("BlockLink.qml");
				blockLinkComponent.createObject(scene.contentItem, {
					x: path_start.x,
					y: path_start.y,
					z: 0, // FIXME: why z=0 does not lead to the path being in the background?
					width: linkingPath.width,
					rotationAngle: linkingPath.rotationAngle,
					sourceBlock: parent,
					destinationBlock: destBlock
				});
			}
		}
	}

	// drag
	MouseArea {
		id: dragArea
		anchors.fill: parent
		drag.target: block
		scrollGestureEnabled: false  // 2-finger-flick gesture should pass through to the Flickable

		// last mouse position in scene coordinates
		property var lastMouseScenePos

		onPressed: {
			// within inner radius
			mouse.accepted = function () {
				var dx = mouse.x - 128;
				var dy = mouse.y - 128;
				return dx*dx+dy*dy < 99*99;
			} ();
			if (mouse.accepted) {
				// store pressed position
				lastMouseScenePos = mapToItem(scene, mouse.x, mouse.y);
				// make this element visible
				if (block.z < scene.highestZ) {
					block.z = ++scene.highestZ;
				}
			}
		}

		// TODO: add drag update step and move links

		onPositionChanged: {
			var mouseScenePos = mapToItem(scene, mouse.x, mouse.y);
			if (drag.active) {
				// TODO: move links
				for (var i = 0; i < scene.contentItem.children.length; ++i) {
					var child = scene.contentItem.children[i];
					if (child && !child.blockName) {
						var ddx = mouseScenePos.x - lastMouseScenePos.x;
						var ddy = mouseScenePos.y - lastMouseScenePos.y;
						if (child.sourceBlock == parent) {
							// source is moving
							child.x += ddx
							child.y += ddy
							var dx = child.width*Math.cos(toRadians(child.rotationAngle)) - ddx;
							var dy = child.width*Math.sin(toRadians(child.rotationAngle)) - ddy;
							child.width = Math.sqrt(dx*dx + dy*dy);
							child.rotationAngle = toDegrees(Math.atan2(dy, dx));
						}
						if (child.destinationBlock == parent) {
							// destination is moving
							var dx = child.width*Math.cos(toRadians(child.rotationAngle)) + ddx;
							var dy = child.width*Math.sin(toRadians(child.rotationAngle)) + ddy;
							child.width = Math.sqrt(dx*dx + dy*dy);
							child.rotationAngle = toDegrees(Math.atan2(dy, dx));
						}
					}
				}
			}
			// update mouse last values
			lastMouseScenePos = mouseScenePos;
		}

		onReleased: {
			scene.setContentSize();
		}
	}
}

