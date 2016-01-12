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

	property bool highlight: false // whether this block is highlighted for link creation
	property Item highlightedBlock: null // other block that is highlighted for link creation
	property bool execTrue: true // whether this block execution was true

	property bool isStarting: true // whether this block is a starting block

	readonly property real centerRadius: 93
	readonly property Item linkingArrow: linkingArrow
	readonly property StartIndicator startIndicator: startIndicator

	// link indicator
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

	// ring for linking and highlighting
	Image {
		source: highlight ? "images/bgHighlight.svg" : (execHighlightTimer.highlighted ? ( execTrue ? "images/bgExec.svg" : "images/bgExecFalse.svg") : "images/bgDefault.svg")
	}

	// starting indicator, show if this block is the start of its click
	StartIndicator {
		id: startIndicator
	}

	// highlight for a short while upon execution on the robot
	HighlightTimer {
		id: execHighlightTimer
	}
	function exec(isTrue) {
		execTrue = isTrue;
		execHighlightTimer.highlight();
	}

	// center background
	Image {
		id: centerImageId
		source: definition.type === "event" ? "images/eventCenter.svg" : "images/actionCenter.svg"
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

	function isLinkTargetValid(destBlock) {
		// do we have a valid block
		if (destBlock && destBlock !== this) {
			// check that this connection does not already exist!
			for (var i = 0; i < linkContainer.children.length; ++i) {
				var link = linkContainer.children[i];
				// if so, return
				if (link.sourceBlock === this && link.destBlock === destBlock) {
					return false;
				}
			}
			return true;
		} else {
			return false;
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
			if (parent.isLinkTargetValid(destBlock)) {
				if (highlightedBlock && highlightedBlock !== destBlock) {
					highlightedBlock.highlight = false;
				}
				highlightedBlock = destBlock;
				highlightedBlock.highlight = true;
			} else if (highlightedBlock) {
				highlightedBlock.highlight = false;
				highlightedBlock = null;
			}
		}

		onReleased: {
			linkingPath.visible = false;
			linkingArrow.visible = false;
			if (highlightedBlock) {
				// prepare scene
				scene.joinClique(parent, highlightedBlock);
				// create link
				var link = blockLinkComponent.createObject(linkContainer, {
					sourceBlock: parent,
					destBlock: highlightedBlock
				});
				// dehighlight block
				highlightedBlock.highlight = false;
				highlightedBlock = null;
			} else {
				// create block
				var pos = mapToItem(blockContainer, mouse.x, mouse.y);
				var newBlock = scene.createBlock(pos.x, pos.y);

				// create link
				newBlock.isStarting = false;
				var link = blockLinkComponent.createObject(linkContainer, {
					sourceBlock: parent,
					destBlock: newBlock
				});
			}
		}
	}

	// we use a timer to have smooth effect affects
	BlockAcceleration {
		id: accelerationTimer
	}

	// drag
	MouseArea {
		id: dragArea
		anchors.fill: parent
		drag.target: block
		scrollGestureEnabled: false  // 2-finger-flick gesture should pass through to the Flickable

		// last mouse position in scene coordinates
		property var prevMousePos

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
				accelerationTimer.startEstimation();
				bringBlockToFront();
			}
		}

		onPositionChanged: {
			if (drag.active) {
				// compute and accumulate displacement for inertia
				var mousePos = mapToItem(blockContainer, mouse.x, mouse.y);
				accelerationTimer.updateEstimation(mousePos.x - prevMousePos.x, mousePos.y - prevMousePos.y);
				prevMousePos = mousePos;
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
			if (delBlock.state === "HIGHLIGHTED") {
				// yes, collect all links and arrows from/to this block
				var toDelete = []
				for (var i = 0; i < linkContainer.children.length; ++i) {
					var child = linkContainer.children[i];
					// if so, collect for removal
					if (child.sourceBlock === block || child.destBlock === block)
						toDelete.push(child);
				}
				// remove collected links and arrows
				for (i = 0; i < toDelete.length; ++i)
					toDelete[i].destroy();
				// remove this block from the scene
				block.destroy();
			} else {
				// no, compute displacement and start timer for inertia
				var mousePos = mapToItem(blockContainer, mouse.x, mouse.y);
				accelerationTimer.updateEstimation(mousePos.x - prevMousePos.x, mousePos.y - prevMousePos.y);
				accelerationTimer.startAcceleration();
			}
			// in any case, set back the delete item to normal state
			delBlock.state = "NORMAL";
		}

		onClicked: {
			editor.block = block;
		}
	}
}

