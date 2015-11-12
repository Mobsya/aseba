import QtQuick 2.5
import QtGraphicalEffects 1.0

Item {
	id: block;
	width: 256
	height: 256
	z: 1

	property string blockName
	property string bgImage
	property string centerImage

	property bool highlight: false
	property Item highlightedBlock: null

	LinkingPath {
		id: linkingPath
		visible: false
	}
	Image {
		id: linkingArrow
		source: "images/linkEndArrow.svg"
		visible: false
	}

	Image {
		source: bgImage
	}

	Image {
		id: centerImageId
		source: centerImage
	}

	DropShadow {
		visible: highlight
		anchors.fill: centerImageId
		color: "#ffffff"
		radius: 16
		samples: 32
		spread: 0.9
		fast: true
		source: centerImageId
	}

	function bringBlockToFront() {
		// make this element visible
		if (block.z < scene.highestZ) {
			block.z = ++scene.highestZ;
		}
	}

	// TODO: move somewhere else
	function toDegrees (angle) {
		return angle * (180 / Math.PI);
	}
	function toRadians (angle) {
		return angle * Math.PI / 180;
	}

	function updateLinkPositions() {
		// TODO: move this into links and arrows
		for (var i = 0; i < linkContainer.children.length; ++i) {
			var child = linkContainer.children[i];
			// if child is part of a link
			if ((child.sourceBlock != block) && (child.destBlock != block))
				continue
			// get new link coordinates
			var sourceBlockCenter = child.sourceBlock.mapToItem(scene.contentItem, child.sourceBlock.width/2, child.sourceBlock.height/2);
			var destBlockCenter = child.destBlock.mapToItem(scene.contentItem, child.destBlock.width/2, child.destBlock.height/2);
			var dx = destBlockCenter.x - sourceBlockCenter.x;
			var dy = destBlockCenter.y - sourceBlockCenter.y;
			var linkAngle = Math.atan2(dy, dx);
			var linkWidth = Math.sqrt(dx*dx + dy*dy);
			// update items
			if (child.linkName == "link") {
				child.x = sourceBlockCenter.x;
				child.y = sourceBlockCenter.y;
				child.setLength(linkWidth);
				child.rotationAngle = toDegrees(linkAngle);
			}
			var arrowDistToCenter = 99+16;
			var gamma = Math.acos(arrowDistToCenter * 0.5 / linkWidth);
			var arcAngle = Math.PI - 2 * gamma;
			var ax = sourceBlockCenter.x + linkWidth * Math.cos(linkAngle - Math.PI/3) + linkWidth * Math.cos(linkAngle + Math.PI/3 + arcAngle);
			var ay = sourceBlockCenter.y + linkWidth * Math.sin(linkAngle - Math.PI/3) + linkWidth * Math.sin(linkAngle + Math.PI/3 + arcAngle);
			var arrowAngle = linkAngle + Math.PI / 3 + arcAngle - Math.PI / 2;
			if (child.linkName == "arrow") {
				child.x = ax - 16;
				child.y = ay - 16;
				child.rotation = toDegrees(arrowAngle);
			}
		}
	}

	// we use a timer to avoid excess calls to updateLinkPositions
	Timer {
		id: nextLinkPositionsUpdate
		interval: 0
		onTriggered: updateLinkPositions()
	}

	onXChanged: nextLinkPositionsUpdate.start()

	onYChanged: nextLinkPositionsUpdate.start()

	// link
	MouseArea {
		id: linkArea
		anchors.fill: parent
		scrollGestureEnabled: false  // 2-finger-flick gesture should pass through to the Flickable

		function updateLinkingPath(mx, my) {
			var cx = width/2;
			var cy = height/2;
			var dx = mx - cx;
			var dy = my - cy;
			var linkAngle = Math.atan2(dy, dx);
			linkingPath.x = cx;// + Math.cos(linkAngle)*128;
			linkingPath.y = cy;// + Math.sin(linkAngle)*128;
			linkingPath.setLength(Math.sqrt(dx*dx + dy*dy)); // - 128 - 32);
			linkingPath.rotationAngle = toDegrees(linkAngle);
			//linkingArrow.x = cx + Math.cos(linkAngle) * (128+16+linkingPath.width) - 16;
			//linkingArrow.y = cy + Math.sin(linkAngle) * (128+16+linkingPath.width) - 16;
			linkingArrow.x = cx + Math.cos(linkAngle) * linkingPath.width - 16;
			linkingArrow.y = cy + Math.sin(linkAngle) * linkingPath.width - 16;
			linkingArrow.rotation = toDegrees(linkAngle);
		}

		function isLinkTargetValid(destBlock) {
			// do we have a valid block
			if (destBlock && destBlock.blockName) {
				// check that this connection does not already exist!
				for (var i = 0; i < linkContainer.children.length; ++i) {
					var child = linkContainer.children[i];
					// if so, return
					if (child.linkName && child.linkName == "link" && child.sourceBlock == parent && child.destBlock == destBlock) {
						return false;
					}
				}
				return true;
			} else {
				return false;
			}
		}

		onPressed: {
			// within inner radius
			var dx = mouse.x - 128;
			var dy = mouse.y - 128;
			if (dx*dx + dy*dy < 128*128) {
				mouse.accepted = true;
				updateLinkingPath(mouse.x, mouse.y);
				linkingPath.visible = true;
				linkingArrow.visible = true;
				bringBlockToFront();
			}
		}

		onPositionChanged: {
			updateLinkingPath(mouse.x, mouse.y);
			var scenePos = mapToItem(blockContainer, mouse.x, mouse.y);
			var destBlock = blockContainer.childAt(scenePos.x, scenePos.y);
			if (destBlock && destBlock.blockName && destBlock != parent) {
				// highlight destblock
				if (highlightedBlock && highlightedBlock != destBlock) {
					highlightedBlock.highlight = false;
				}
				if (isLinkTargetValid(destBlock)) {
					destBlock.highlight = true;
					highlightedBlock = destBlock;
				}
			} else if (highlightedBlock) {
				highlightedBlock.highlight = false;
			}
		}

		onReleased: {
			linkingPath.visible = false;
			linkingArrow.visible = false;
			if (highlightedBlock) {
				highlightedBlock.highlight = false;
			}

			var scenePos = mapToItem(blockContainer, mouse.x, mouse.y);
			var destBlock = blockContainer.childAt(scenePos.x, scenePos.y);

			if (isLinkTargetValid(destBlock)) {
				// create link
				var thisBlockCenter = mapToItem(linkContainer, width/2, height/2);
				var thatBlockCenter = destBlock.mapToItem(linkContainer, destBlock.width/2, destBlock.height/2);
				var dx = thatBlockCenter.x - thisBlockCenter.x;
				var dy = thatBlockCenter.y - thisBlockCenter.y;
				var linkAngle = Math.atan2(dy, dx);
				var linkWidth = Math.sqrt(dx*dx + dy*dy);
				var blockLinkComponent = Qt.createComponent("BlockLink.qml");
				blockLinkComponent.createObject(linkContainer, {
					x: thisBlockCenter.x,
					y: thisBlockCenter.y,
					z: 0,
					width: linkWidth,
					rotationAngle: toDegrees(linkAngle),
					sourceBlock: parent,
					destBlock: destBlock,
					trim: true
				});
				// create end arrow
				var arrowDistToCenter = 99+16;
				var gamma = Math.acos(arrowDistToCenter * 0.5 / linkWidth);
				var arcAngle = Math.PI - 2 * gamma;
				var ax = thisBlockCenter.x + linkWidth * Math.cos(linkAngle - Math.PI/3) + linkWidth * Math.cos(linkAngle + Math.PI/3 + arcAngle);
				var ay = thisBlockCenter.y + linkWidth * Math.sin(linkAngle - Math.PI/3) + linkWidth * Math.sin(linkAngle + Math.PI/3 + arcAngle);
				var arrowAngle = linkAngle + Math.PI / 3 + arcAngle - Math.PI / 2;
				var blockLinkArrowComponent = Qt.createComponent("BlockLinkArrow.qml");
				blockLinkArrowComponent.createObject(linkContainer, {
					x: ax - 16,
					y: ay - 16,
					z: 1,
					rotation: toDegrees(arrowAngle),
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

		onPressed: {
			// within inner radius
			mouse.accepted = function () {
				var dx = mouse.x - 128;
				var dy = mouse.y - 128;
				return dx*dx+dy*dy < 99*99;
			} ();
			if (mouse.accepted) {
				bringBlockToFront();
			}
		}

		onPositionChanged: {
			if (drag.active) {
				updateLinkPositions();
			}
		}

		onReleased: {
			scene.setContentSize();
		}
	}
}

