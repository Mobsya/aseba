import QtQuick 2.5
import QtGraphicalEffects 1.0
import "utils.js" as Utils

DropArea {
	id: block;

	width: 256
	height: 256
	z: 1

	property var id // used in serialization/deserialization
	property string typeRestriction: "" // "", "event", "action"
	property BlockDefinition definition
	property var params

	property bool canDelete: true // whether this block can be deleted
	property bool canGraph: true // whether this block can create links and move startIndicator
	property bool canDrag: canDelete || canGraph;

	property bool isError: false // whether this block is involved in an error
	property bool isExec: false // whether this block is currently executing

	property bool highlight: containsDrag // whether this block is highlighted for link creation
	property Item highlightedBlock: null // other block that is highlighted for link creation
	property bool execTrue: true // whether this block execution was true

	property bool isStarting: true // whether this block is a starting block

	readonly property real centerRadius: 93
	readonly property Item linkingArrow: linkingArrow
	readonly property StartIndicator startIndicator: startIndicator

	onEntered: handleDrag(drag)
	onPositionChanged: handleDrag(drag)
	onDropped: scene.handleBlockDrop(this, drop)
	function handleDrag(drag) {
		var dx = drag.x - width / 2;
		var dy = drag.y - height / 2;
		var dr = width / 2;
		drag.accepted = (dx*dx + dy*dy) < (dr*dr);
	}

	// link indicator
	Rectangle {
		id: linkingPath
		color: "#9478aa"
		width: 0
		height: 10
		transformOrigin: "Left"
		visible: false
	}
	HDPIImage {
		id: linkingArrow
		source: "images/linkEndArrow.svg"
		width: 32 // working around Qt bug with SVG and HiDPI
		height: 32 // working around Qt bug with SVG and HiDPI
		visible: false
	}

	// ring for linking and highlighting
	HDPIImage {
		source: isError ? "images/bgError.svg" :
			(highlight ? "images/bgHighlight.svg" :
				((isExec || execHighlightTimer.highlighted) ?
					( execTrue ? "images/bgExec.svg" : "images/bgExecFalse.svg") :
					( definition === null && !canGraph ? "" : "images/bgDefault.svg")
				)
			)
		width: 256 // working around Qt bug with SVG and HiDPI
		height: 256 // working around Qt bug with SVG and HiDPI
	}

	// starting indicator, show if this block is the start of its click
	StartIndicator {
		id: startIndicator
		enabled: canGraph
	}

	// highlight for a short while upon execution on the robot
	HighlightTimer {
		id: execHighlightTimer
	}
	function exec() {
		execHighlightTimer.highlight();
	}

	function serialize(id, withCoord) {
		block.id = id;
		var definition = block.definition.objectName;
		var params = block.params;
		var isStarting = block.isStarting;
		if (withCoord) {
			var pos = mapToItem(scene, 0, 0);
			return {
				x: pos.x,
				y: pos.y,
				definition: definition,
				params: params,
				isStarting: isStarting,
			};
		} else {
			return {
				definition: definition,
				params: params,
				isStarting: isStarting,
			};
		}
	}

	// center background
	BlockBackground {
		id: centerImageId
		typeRestriction: block.typeRestriction
		definition: block.definition
		anchors.centerIn: parent
	}

	// miniature
	Item {
		id: placeholder // for miniature
		enabled: false;
		anchors.centerIn: parent
		scale: 0.72
		width: 256 // working around Qt bug with SVG and HiDPI
		height: 256 // working around Qt bug with SVG and HiDPI
	}
	// recreate miniature on params changed
	onParamsChanged: {
		placeholder.children = [];
		if (definition !== null) {
			definition.miniature.createObject(placeholder, {"params": params, "anchors.centerIn": placeholder});
		}
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
		enabled: canGraph
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
				// create block using the last placed one
				var pos = mapToItem(blockContainer, mouse.x, mouse.y);
				var newBlock = scene.createBlock(pos.x, pos.y, block.definition);

				// create link
				newBlock.isStarting = false;
				var link = blockLinkComponent.createObject(linkContainer, {
					sourceBlock: parent,
					destBlock: newBlock
				});

				// directly edit block
				blockEditor.setBlock(newBlock);
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
		drag.target: canDrag ? block : null
		scrollGestureEnabled: false  // 2-finger-flick gesture should pass through to the Flickable

		property real startX
		property real startY

		onPressed: {
			// within inner radius?
			var dx = mouse.x - 128;
			var dy = mouse.y - 128;
			var accepted = dx*dx+dy*dy < centerRadius*centerRadius;
			mouse.accepted = accepted;

			if (accepted && canDrag) {
				// if so...
				if (canGraph) {
					var mousePos = mapToItem(blockContainer, mouse.x, mouse.y);
					accelerationTimer.startEstimation(mousePos);
				}
				if (canDelete) {
					startX = block.x;
					startY = block.y;
				}
				bringBlockToFront();
			}
		}

		onPositionChanged: {
			if (drag.active) {
				if (canGraph) {
					// compute and accumulate displacement for inertia
					var mousePos = mapToItem(blockContainer, mouse.x, mouse.y);
					accelerationTimer.updateEstimation(mousePos);
				}

				if (canDelete) {
					// show trash bin
					eventPane.showTrash = true;
					actionPane.showTrash = true;
					// check whether we are hovering delete block item
					eventPane.trashOpen = eventPane.contains(mapToItem(eventPane, mouse.x, mouse.y));
					actionPane.trashOpen = actionPane.contains(mapToItem(actionPane, mouse.x, mouse.y));
				}
			}
		}

		onReleased: {
			if (eventPane.trashOpen || actionPane.trashOpen) {
				// to be deleted
				scene.deleteBlock(block);
			} else if (canGraph) {
				// compute displacement and start timer for inertia
				var mousePos = mapToItem(blockContainer, mouse.x, mouse.y);
				accelerationTimer.updateEstimation(mousePos);
				accelerationTimer.startAcceleration();
			}

			if (canDelete) {
				// go back to initial position
				block.x = startX;
				block.y = startY;
				// in any case, hide back the delete icons
				eventPane.clearTrash();
				actionPane.clearTrash();
			}
		}

		onClicked: {
			blockEditor.setBlock(block);
		}
	}
}

