import QtQuick 2.5
import QtGraphicalEffects 1.0

Item {
	id: block;

	width: 256
	height: 256
	z: 1

	property string type
	property string name
	property var params: null
	property Item miniature: null

	property bool highlight: false
	property Item highlightedBlock: null

	property real vx: 0 // in px per millisecond
	property real vy: 0 // in px per millisecond

	Rectangle {
		id: linkingPath
		color: "#eceded"
		width: 0
		height: 10
		transformOrigin: "Left"
		visible: false
	}
	Image {
		id: linkingArrow
		source: "images/linkEndArrow.svg"
		visible: false
	}

	Image {
		source: type == "event" ? "images/eventBg.svg" : "images/actionBg.svg"
	}

	Image {
		id: centerImageId
		source: type == "event" ? "images/eventCenter.svg" : "images/actionCenter.svg"
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
	function sign(v) {
		if (v > 0)
			return 1;
		else if (v < 0)
			return -1;
		else
			return 0;
	}

	function updateLinkPositions() {
		// TODO: move this into links and arrows
		for (var i = 0; i < linkContainer.children.length; ++i) {
			var child = linkContainer.children[i];
			// if child is part of a link
			if ((child.sourceBlock != block) && (child.destBlock != block))
				continue
			// get new link coordinates
			var sourceBlockCenter = child.sourceBlock.mapToItem(scene, child.sourceBlock.width/2, child.sourceBlock.height/2);
			var destBlockCenter = child.destBlock.mapToItem(scene, child.destBlock.width/2, child.destBlock.height/2);
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

	// we use a timer to have some smooth effect
	Timer {
		id: accelerationTimer
		interval: 17
		repeat: true
		onTriggered: {
			x += (vx * interval) * 0.001;
			y += (vy * interval) * 0.001;
			vx *= 0.85;
			vy *= 0.85;
			if (Math.abs(vx) < 1 && Math.abs(vy) < 1) {
				running = false;
				vx = 0;
				vy = 0;
			}
		}
	}

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
			linkingPath.x = cx;
			linkingPath.y = cy - linkingPath.height*0.5;
			linkingPath.width = Math.sqrt(dx*dx + dy*dy);
			linkingPath.rotation = toDegrees(linkAngle);
			linkingArrow.x = cx + Math.cos(linkAngle) * linkingPath.width - 16;
			linkingArrow.y = cy + Math.sin(linkAngle) * linkingPath.width - 16;
			linkingArrow.rotation = toDegrees(linkAngle);
		}

		function isLinkTargetValid(destBlock) {
			// do we have a valid block
			if (destBlock && destBlock.name) {
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
			if (destBlock && destBlock.name && destBlock != parent) {
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

		// last mouse position in scene coordinates
		property var prevMousePos
		property double prevMouseTime: 0

		onPressed: {
			// within inner radius?
			mouse.accepted = function () {
				var dx = mouse.x - 128;
				var dy = mouse.y - 128;
				return dx*dx+dy*dy < 99*99;
			} ();
			// if so...
			if (mouse.accepted) {
				prevMousePos = mapToItem(blockContainer, mouse.x, mouse.y);
				prevMouseTime = new Date().valueOf();
				bringBlockToFront();
			}
		}

		onPositionChanged: {
			if (drag.active) {
				// update links
				updateLinkPositions();
				// compute and accumulate displacement for inertia
				var mousePos = mapToItem(blockContainer, mouse.x, mouse.y);
				var now = new Date().valueOf();
				var dt = now - prevMouseTime;
				block.vx = block.vx * 0.6 + (mousePos.x - prevMousePos.x) * 0.4 * dt;
				block.vy = block.vy * 0.6 + (mousePos.y - prevMousePos.y) * 0.4 * dt;
				prevMousePos = mousePos;
				prevMouseTime = now;
				// check whether we are hovering delete block item
				var delBlockPos = mapToItem(delBlock, mouse.x, mouse.y);
				if (delBlock.contains(delBlockPos))
					delBlock.state = "HIGHLIGHTED";
				else
					delBlock.state = "NORMAL";
			}
		}

		onReleased: {
			// to be deleted?
			if (delBlock.state == "HIGHLIGHTED") {
				// yes, collect all links and arrows from/to this block
				var toDelete = []
				for (var i = 0; i < linkContainer.children.length; ++i) {
					var child = linkContainer.children[i];
					// if so, collect for removal
					if (child.sourceBlock == block || child.destBlock == block)
						toDelete.push(child);
				}
				// remove collected links and arrows
				for (i = 0; i < toDelete.length; ++i)
					toDelete[i].parent = null;
				// remove this block from the scene
				block.parent = null;
			} else {
				// no, compute displacement and start timer for inertia
				var mousePos = mapToItem(blockContainer, mouse.x, mouse.y);
				var now = new Date().valueOf();
				var dt = now - prevMouseTime;
				block.vx = block.vx * 0.6 + (mousePos.x - prevMousePos.x) * 0.4 * dt;
				block.vy = block.vy * 0.6 + (mousePos.y - prevMousePos.y) * 0.4 * dt;
				accelerationTimer.running = true;
			}
			// in any case, set back the delete item to normal state
			delBlock.state = "NORMAL";
		}

		onClicked: {
			editor.setEditorItem(miniature.createEditor(editor));
			editor.editedBlock = parent;
			editor.visible = true;
		}
	}
}

