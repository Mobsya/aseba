import QtQuick 2.0
import "utils.js" as Utils

Canvas {
	z: 0

	readonly property real leftRadius: 133
	readonly property real rightRadius: 118
	readonly property real arrowRadius: 115

	property Item sourceBlock: Null
	property Item destBlock: Null

	property bool isError: false // whether this link is involved in an error

	property Item highlightedBlock: null // new block that is highlighted for link creation

	// we update our position when attached blocks are moved
	Connections {
		target: sourceBlock
		onXChanged: nextLinkPositionsUpdate.start()
		onYChanged: nextLinkPositionsUpdate.start()
	}
	Connections {
		target: destBlock
		onXChanged: nextLinkPositionsUpdate.start()
		onYChanged: nextLinkPositionsUpdate.start()
	}
	// we use a timer to avoid excess calls to updatePosition
	Timer {
		id: nextLinkPositionsUpdate
		interval: 0
		onTriggered: updatePosition()
	}

	property real lastPaintedWidth: 0

	property bool isElse: false
	property bool canBeElse: sourceBlock && sourceBlock.definition.type === "event"

	onIsElseChanged: {
		requestPaint();
		compiler.compile();
	}
	onCanBeElseChanged: {
		if (!canBeElse)
			isElse = false;
	}

	// highlight for a short while upon execution on the robot
	HighlightTimer {
		id: execHighlightTimer
		onHighlightedChanged: parent.requestPaint()
	}
	function exec() {
		execHighlightTimer.highlight();
	}

	// return a JSON representation of the content of the link
	function serialize() {
		return {
			"isElse": isElse
		}
	}

	// to force images used in context2d to be loaded upon initalization
	Image {
		source: "images/elsePattern.png"
		visible: false
	}
	Image {
		source: "images/elsePatternExec.png"
		visible: false
	}
	Image {
		source: "images/elsePatternError.png"
		visible: false
	}

	HDPIImage {
		id: arrow
		source: isError ? "images/linkEndArrowError.svg" : (execHighlightTimer.highlighted ? "images/linkEndArrowExec.svg" : "images/linkEndArrow.svg")
		width: 32 // working around Qt bug with SVG and HiDPI
		height: 32 // working around Qt bug with SVG and HiDPI
		visible: parent.width > 256
	}

	transformOrigin: Item.BottomLeft

	Component.onCompleted: {
		updatePosition();
	}

	function updatePosition() {
		var sourceBlockCenter = sourceBlock.mapToItem(scene, sourceBlock.width/2, sourceBlock.height/2);
		var destBlockCenter = destBlock.mapToItem(scene, destBlock.width/2, destBlock.height/2);
		var dx = destBlockCenter.x - sourceBlockCenter.x;
		var dy = destBlockCenter.y - sourceBlockCenter.y;
		width = Math.sqrt(dx*dx + dy*dy);
		height = width / 6;
		x = sourceBlockCenter.x;
		y = sourceBlockCenter.y - height;
		var linkAngle = Math.atan2(dy, dx);
		rotation = Utils.toDegrees(linkAngle);
		var arrowDistToCenter = arrowRadius+16;
		var gamma = Math.acos(arrowDistToCenter * 0.5 / width);
		var arcAngle = Math.PI - 2 * gamma;
		var ax = width * Math.cos(Math.PI/3) + width * Math.cos(- Math.PI/3 - arcAngle);
		var ay = width * Math.sin(Math.PI/3) + width * Math.sin(- Math.PI/3 - arcAngle);
		var arrowAngle = - Math.PI / 3 - arcAngle + Math.PI / 2;
		arrow.x = ax - 16;
		arrow.y = ay - 16 + height;
		arrow.rotation = Utils.toDegrees(arrowAngle);
		if (!elseDeleteDoodleMouseArea.drag.active)
			elseDeleteDoodle.resetPosition();
		// only redraw if width has changed more than 2 pixels
		if (Math.abs(width - lastPaintedWidth) > 2)
			requestPaint();
	}

	onPaint: {
		lastPaintedWidth = width;
		// compute start and end positions
		var localLeftRadius = leftRadius;
		var localRightRadius = rightRadius;
		var ctx = getContext('2d');
		ctx.clearRect(0, 0, width, height);
		if (width < 228) {
			return;
		} else if (width < 256) {
			localLeftRadius = rightRadius;
		}
		var leftGamma = Math.acos(localLeftRadius * 0.5 / width);
		var leftArcAngle = Math.PI - 2 * leftGamma;
		var rightGamma = Math.acos(localRightRadius * 0.5 / width);
		var rightArcAngle = Math.PI - 2 * rightGamma;

		// draw the arc
		ctx.lineWidth = 10;
		ctx.lineCap = "square";
		if (isElse) {
			if (isError) {
				ctx.strokeStyle = ctx.createPattern("images/elsePatternError.png", "repeat");
			} else {
				if (execHighlightTimer.highlighted)
					ctx.strokeStyle = ctx.createPattern("images/elsePatternExec.png", "repeat");
				else
					ctx.strokeStyle = ctx.createPattern("images/elsePattern.png", "repeat");
			}
		} else {
			if (isError) {
				ctx.strokeStyle = "#f52300";
			} else {
				if (execHighlightTimer.highlighted)
					ctx.strokeStyle = "#F5E800";
				else
					ctx.strokeStyle = "#9478aa";
			}
		}
		ctx.beginPath();
		ctx.arc(width/2, height+Math.sqrt(width*width*3/4), width, 4*Math.PI/3+rightArcAngle, 5*Math.PI/3-leftArcAngle);
		ctx.stroke();
	}

	HDPIImage {
		id: elseDeleteDoodle

		visible: parent.width > 228 + 96
		source: isError ?
			(canBeElse ?
				(isElse ? "images/arrowOptionActiveFalseError.svg" : "images/arrowOptionActiveTrueError.svg") :
				"images/arrowOptionInactiveError.svg") :
			(canBeElse ?
				(isElse ?
					(execHighlightTimer.highlighted ? "images/arrowOptionActiveFalseExec.svg" : "images/arrowOptionActiveFalse.svg") :
					(execHighlightTimer.highlighted ? "images/arrowOptionActiveTrueExec.svg" : "images/arrowOptionActiveTrue.svg")
				) :
				(execHighlightTimer.highlighted ? "images/arrowOptionInactiveExec.svg" : "images/arrowOptionInactive.svg")
			)
		width: 50 // working around Qt bug with SVG and HiDPI
		height: 50 // working around Qt bug with SVG and HiDPI
		z: 1

		function resetPosition() {
			x = parent.width/2-25;
			y = parent.height*(1.-0.805)-25;
		}

		MouseArea {
			id: elseDeleteDoodleMouseArea
			anchors.fill: parent
			drag.target: elseDeleteDoodle

			onClicked: {
				if (canBeElse)
					isElse = !isElse;
			}

			onPressed: { }

			onPositionChanged: {
				var scenePos = mapToItem(blockContainer, mouse.x, mouse.y);
				var destBlock = blockContainer.childAt(scenePos.x, scenePos.y);

				// show trash bin
				eventPane.showTrash = true;
				actionPane.showTrash = true;
				// elseDeleteDoodle.z = 1; // FIXME: not working

				// check whether we are hovering delete block item
				eventPane.trashOpen = eventPane.contains(mapToItem(eventPane, mouse.x, mouse.y));
				actionPane.trashOpen = actionPane.contains(mapToItem(actionPane, mouse.x, mouse.y));
				// check whether we are hovering delete block item
				if (eventPane.trashOpen || actionPane.trashOpen) {
					if (highlightedBlock) {
						highlightedBlock.highlight = false;
						highlightedBlock = null;
					}
				// check whether we are hovering another block
				} else if (sourceBlock.isLinkTargetValid(destBlock)) {
					if (highlightedBlock && highlightedBlock !== destBlock) {
						highlightedBlock.highlight = false;
						highlightedBlock = null;
					}
					highlightedBlock = destBlock;
					highlightedBlock.highlight = true;
				// we are not hovering anything
				} else {
					if (highlightedBlock) {
						highlightedBlock.highlight = false;
						highlightedBlock = null;
					}
				}
			}

			onReleased: {
				// if to be deleted, destroy this link
				if (eventPane.trashOpen || actionPane.trashOpen) {
					scene.removeLink(parent.parent);
				// if to be moved to another block, create new link and destroy this link
				} else if (highlightedBlock) {
					highlightedBlock.highlight = false;
					scene.joinClique(sourceBlock, highlightedBlock, parent.parent);
					var link = blockLinkComponent.createObject(linkContainer, {
						sourceBlock: sourceBlock,
						destBlock: highlightedBlock,
						isElse: isElse
					});
					scene.removeLink(parent.parent);
				} else {
					parent.resetPosition();
				}
				// in any case, hide back the delete icons
				eventPane.clearTrash();
				actionPane.clearTrash();
				//block.parent.z = 0;
			}
		}
	}
}
