import QtQuick 2.0
import "utils.js" as Utils

Canvas {
	readonly property real leftRadius: 133
	readonly property real rightRadius: 118
	readonly property real arrowRadius: 115

	property Item sourceBlock: Null
	property Item destBlock: Null

	property bool execHighlight: false

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

	onIsElseChanged: requestPaint();
	onCanBeElseChanged: {
		if (!canBeElse)
			isElse = false;
		elseDeleteDoodle.requestPaint();
	}

	// highlight execution for a short while
	Timer {
		id: execHighlightTimer
		interval: 100
		onTriggered: {
			execHighlight = false;
			requestPaint();
		}
	}
	function exec() {
		execHighlight = true;
		execHighlightTimer.restart();
		requestPaint();
	}

	// to force the image to be loaded upon initalization
	Image {
		source: "images/elsePattern.png"
		visible: false
	}
	Image {
		source: "images/elsePatternExec.png"
		visible: false
	}

	Image {
		id: arrow
		source: "images/linkEndArrow.svg"
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
			if (execHighlight)
				ctx.strokeStyle = ctx.createPattern("images/elsePatternExec.png", "repeat");
			else
				ctx.strokeStyle = ctx.createPattern("images/elsePattern.png", "repeat");
		} else {
			if (execHighlight)
				ctx.strokeStyle = "#E285FF";
			else
				ctx.strokeStyle = "#a2d8dc";
		}
		ctx.beginPath();
		ctx.arc(width/2, height+Math.sqrt(width*width*3/4), width, 4*Math.PI/3+rightArcAngle, 5*Math.PI/3-leftArcAngle);
		ctx.stroke();
	}

	Canvas {
		id: elseDeleteDoodle
		visible: parent.width > 228 + 96
		width: 96
		height: 32
		z: -1

		function resetPosition() {
			x = parent.width/2-48;
			y = parent.height*(1.-0.805);
		}

		onPaint: {
			var ctx = getContext('2d');
			if (canBeElse) {
				ctx.fillStyle = "#f48574";
			} else {
				ctx.fillStyle = "#a0a0a0";
			}
			ctx.beginPath();
			ctx.moveTo(0, 0);
			ctx.bezierCurveTo(0, 16, 24, 32, 48, 32);
			ctx.bezierCurveTo(72, 32, 96, 16, 96, 0);
			ctx.fill();
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
				// check whether we are hovering delete block item
				var delBlockPos = mapToItem(delBlock, mouse.x, mouse.y);
				if (delBlock.contains(delBlockPos))
					delBlock.state = "HIGHLIGHTED";
				else
					delBlock.state = "NORMAL";
			}

			onReleased: {
				// to be deleted?
				if (delBlock.state === "HIGHLIGHTED") {
					parent.parent.destroy();
				} else {
					parent.resetPosition();
				}
				// in any case, set back the delete item to normal state
				delBlock.state = "NORMAL";
			}
		}
	}
}
