import QtQuick 2.5
import QtGraphicalEffects 1.0
import "utils.js" as Utils

Item {
	id: block;

	width: 256
	height: 256
	z: 1

	property BlockDefinition definition
	property var params

	property bool highlight: false
	property Item highlightedBlock: null

	readonly property real centerRadius: 93
	readonly property Item linkingArrow: linkingArrow

	property real vx: 0 // in px per millisecond
	property real vy: 0 // in px per millisecond

	Rectangle {
		id: linkingPath
		color: "#a2d8dc"
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
		id: backgroundImage
		source: highlight ? "images/bgHighlight.svg" : "images/bgDefault.svg"
	}

//	DropShadow {
//		visible: true
//		anchors.fill: backgroundImage
//		color: highlight ? "#e0F05F48" : "#e082CEC6"
//		radius: 5
//		samples: 8
//		spread: 0.5
//		fast: true
//		source: backgroundImage
//	}

	Image {
		id: centerImageId
		source: definition.type == "event" ? "images/eventCenter.svg" : "images/actionCenter.svg"
		anchors.centerIn: parent
		scale: 0.72
	}

	// miniature
	Item {
		id: placeholder // for miniature
		enabled: false;
		anchors.centerIn: parent
		scale: 0.72
	}
	// recreate miniature on params changed
	onParamsChanged: {
		placeholder.children = [];
		definition.miniature.createObject(placeholder, {"params": params, "anchors.centerIn": placeholder});
		compiler.compile();
	}

	function bringBlockToFront() {
		// make this element visible
		if (block.z < scene.highestZ) {
			block.z = ++scene.highestZ;
		}
	}

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

	// link under creation
	MouseArea {
		id: linkArea
		anchors.fill: parent
		scrollGestureEnabled: false  // 2-finger-flick gesture should pass through to the Flickable

		function updateLinkingPath(mx, my) {
			var cx = width/2;
			var cy = height/2;
			var dx = mx - cx;
			var dy = my - cy;
			var length = Math.sqrt(dx*dx + dy*dy);
			var startLength = 113;
			var reducedLength = length - startLength;
			if (reducedLength < 0) {
				linkingPath.visible = false;
				linkingArrow.visible = false;
				return;
			} else {
				linkingPath.visible = true;
				linkingArrow.visible = true;
			}

			var linkAngle = Math.atan2(dy, dx);
			linkingPath.x = cx + Math.cos(linkAngle) * startLength;
			linkingPath.y = cy - linkingPath.height*0.5 + Math.sin(linkAngle) * startLength;
			linkingPath.width = reducedLength;
			linkingPath.rotation = Utils.toDegrees(linkAngle);
			linkingArrow.x = cx + Math.cos(linkAngle) * length - 16;
			linkingArrow.y = cy + Math.sin(linkAngle) * length - 16;
			linkingArrow.rotation = Utils.toDegrees(linkAngle);
		}

		function isLinkTargetValid(destBlock) {
			// do we have a valid block
			if (destBlock && destBlock.parent === blockContainer) {
				// check that this connection does not already exist!
				for (var i = 0; i < linkContainer.children.length; ++i) {
					var link = linkContainer.children[i];
					// if so, return
					if (link.sourceBlock === parent && link.destBlock === destBlock) {
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
				bringBlockToFront();
			}
		}

		onPositionChanged: {
			updateLinkingPath(mouse.x, mouse.y);
			var scenePos = mapToItem(blockContainer, mouse.x, mouse.y);
			var destBlock = blockContainer.childAt(scenePos.x, scenePos.y);
			if (destBlock && destBlock.parent === blockContainer && destBlock != parent) {
				// highlight destblock
				if (highlightedBlock && highlightedBlock !== destBlock) {
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
				var link = blockLinkComponent.createObject(linkContainer, {
					z: 0,
					sourceBlock: parent,
					destBlock: destBlock
				});
				compiler.compile();
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
				return dx*dx+dy*dy < centerRadius*centerRadius;
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
					toDelete[i].destroy();
				// remove this block from the scene
				block.destroy();
				compiler.compile();
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
			editor.block = block;
		}
	}
}

