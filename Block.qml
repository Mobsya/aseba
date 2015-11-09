import QtQuick 2.5

Item {
	id: block;
	width: 256
	height: 256
	z: 1

	property string blockName
	property string bgImage
	property string centerImage

	Image {
		source: bgImage

	}

	Image {
		source: centerImage
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

			var scenePos = mapToItem(scene.contentItem, mouse.x, mouse.y);
			var destBlock = scene.contentItem.childAt(scenePos.x, scenePos.y);

			if (destBlock && destBlock.blockName) {
				// check that this connection does not already exist!
				for (var i = 0; i < scene.contentItem.children.length; ++i) {
					var child = scene.contentItem.children[i];
					// if so, return
					if (child && child.linkName && child.linkName == "link" && child.sourceBlock == parent && child.destBlock == destBlock) {
						return;
					}
				}

				// create link
				var thisBlockCenter = mapToItem(scene.contentItem, width/2, height/2);
				var thatBlockCenter = destBlock.mapToItem(scene.contentItem, destBlock.width/2, destBlock.height/2);
				var dx = thatBlockCenter.x - thisBlockCenter.x;
				var dy = thatBlockCenter.y - thisBlockCenter.y;
				var linkAngle = Math.atan2(dy, dx);
				var linkWidth = Math.sqrt(dx*dx + dy*dy) - 2*128 - 32;
				var blockLinkComponent = Qt.createComponent("BlockLink.qml");
				blockLinkComponent.createObject(scene.contentItem, {
					x: thisBlockCenter.x + Math.cos(linkAngle)*128,
					y: thisBlockCenter.y + Math.sin(linkAngle)*128,
					z: 0, // FIXME: why z=0 does not lead to the path being in the background?
					width: linkWidth,
					rotationAngle: toDegrees(linkAngle),
					sourceBlock: parent,
					destBlock: destBlock
				});
				// create end arrow
				var blockLinkArrowComponent = Qt.createComponent("BlockLinkArrow.qml");
				blockLinkArrowComponent.createObject(scene.contentItem, {
					x: thisBlockCenter.x + Math.cos(linkAngle) * (128+16+linkWidth) - 16,
					y: thisBlockCenter.y + Math.sin(linkAngle) * (128+16+linkWidth) - 16,
					z: 0,
					rotation: toDegrees(linkAngle),
					sourceBlock: parent,
					destBlock: destBlock
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
				lastMouseScenePos = mapToItem(scene.contentItem, mouse.x, mouse.y);
				// make this element visible
				if (block.z < scene.highestZ) {
					block.z = ++scene.highestZ;
				}
			}
		}

		onPositionChanged: {
			var mouseScenePos = mapToItem(scene.contentItem, mouse.x, mouse.y);
			if (drag.active) {
				for (var i = 0; i < scene.contentItem.children.length; ++i) {
					var child = scene.contentItem.children[i];
					// if child is part of a link
					if (child && child.linkName) {
						// get new link coordinates
						var sourceBlockCenter = child.sourceBlock.mapToItem(scene.contentItem, child.sourceBlock.width/2, child.sourceBlock.height/2);
						var destBlockCenter = child.destBlock.mapToItem(scene.contentItem, child.destBlock.width/2, child.destBlock.height/2);
						var dx = destBlockCenter.x - sourceBlockCenter.x;
						var dy = destBlockCenter.y - sourceBlockCenter.y;
						var linkAngle = Math.atan2(dy, dx);
						var linkWidth = Math.sqrt(dx*dx + dy*dy) - 2*128 - 32;
						// update items
						if (child.linkName == "link") {
							child.x = sourceBlockCenter.x + Math.cos(linkAngle)*128;
							child.y = sourceBlockCenter.y + Math.sin(linkAngle)*128;
							child.width = linkWidth;
							child.rotationAngle = toDegrees(linkAngle);
						}
						if (child.linkName == "arrow") {
							child.x = sourceBlockCenter.x + Math.cos(linkAngle) * (128+16+linkWidth) - 16;
							child.y = sourceBlockCenter.y + Math.sin(linkAngle) * (128+16+linkWidth) - 16;
							child.rotation = toDegrees(linkAngle);
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

