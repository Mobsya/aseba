import QtQuick 2.0

Image {
	readonly property real xRest: -width+2
	readonly property real yRest: (parent.height - height) / 2

	x: xRest
	y: yRest
	z: 1
	source: execHighlightTimer.highlighted ? "images/threadStartIndicatorExec.svg" : "images/threadStartIndicator.svg"
	visible: isStarting

	// small rectangle visually linking to the block
	Rectangle {
		color: execHighlightTimer.highlighted ? "#F5E800" : "#a2d8dc"
		x: parent.width-2
		width: 14
		height: 10
		anchors.verticalCenter: parent.verticalCenter
		visible: isStarting && !isStartingIndicatorMouseArea.drag.active
	}

	// drag area with highlight support
	MouseArea {
		id: isStartingIndicatorMouseArea
		anchors.fill: parent
		drag.target: parent

		onPressed: {
			block.bringBlockToFront();
		}

		onPositionChanged: {
			var scenePos = mapToItem(blockContainer, mouse.x, mouse.y);
			var destBlock = blockContainer.childAt(scenePos.x, scenePos.y);
			// we are only allowed to drop on another block of the same clique
			if (destBlock && destBlock !== block && (!highlightedBlock || highlightedBlock !== destBlock) && scene.areBlocksInSameClique(destBlock, block)) {
				if (highlightedBlock && highlightedBlock !== destBlock) {
					highlightedBlock.highlight = false;
				}
				highlightedBlock = destBlock;
				highlightedBlock.highlight = true;
			} else if (highlightedBlock && highlightedBlock !== destBlock) {
				highlightedBlock.highlight = false;
				highlightedBlock = null;
			}
		}

		onReleased: {
			if (highlightedBlock) {
				// exchange starting indicators
				block.isStarting = false;
				highlightedBlock.isStarting = true;
				// dehighlight block
				highlightedBlock.highlight = false;
				highlightedBlock = null;
			}
			privateMethods.resetPosition();
		}
	}

	// highlight for a short while upon execution on the robot
	HighlightTimer {
		id: execHighlightTimer
	}
	function exec() {
		execHighlightTimer.highlight();
	}

	QtObject {
		id: privateMethods

		function resetPosition() {
			x = xRest;
			y = yRest;
		}
	}
}
