import QtQuick 2.5


Item {
	id: block;
	width: 256
	height: 256
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

			var scene_pos = mapToItem(scene, mouse.x, mouse.y);
			var dest_block = scene.contentItem.childAt(scene_pos.x, scene_pos.y);

			if (dest_block && dest_block.blockName) {
				// create connections
				var path_start = mapToItem(scene, linkingPath.x, linkingPath.y)
				var blockLinkComponent = Qt.createComponent("BlockLink.qml");
				blockLinkComponent.createObject(scene.contentItem, {
					x: path_start.x,
					y: path_start.y,
					z: 0, // FIXME: why z=0 does not lead to the path being in the background?
					width: linkingPath.width,
					rotationAngle: linkingPath.rotationAngle,
					sourceBlock: this,
					destinationBlock: dest_block
				});
			}
			// TODO: decide for a model to track the links
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

		// TODO: add drag update step and move links

		onReleased: {
			scene.setContentSize();
		}
	}
}

