import QtQuick 2.7

HDPIImage {
	property bool isError: false // whether this start indicator is involved in an error
	property bool isExec: false

	readonly property real xRest: -width+2
	readonly property real yRest: (parent.height - height) / 2

	x: xRest
	y: yRest
	z: 1
	source: isError ?
		"images/threadStartIndicatorError.svg" :
		(isExec ? "images/threadStartIndicatorExec.svg" : "images/threadStartIndicator.svg")
	width: 92 // working around Qt bug with SVG and HiDPI
	height: 92 // working around Qt bug with SVG and HiDPI
	visible: isStarting

	// small rectangle visually linking to the block
	Rectangle {
		color: isError ?
			"#f52300" :
			(isExec ? "#F5E800" : "#9478aa")
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

	QtObject {
		id: privateMethods

		function resetPosition() {
			x = xRest;
			y = yRest;
		}
	}
}
