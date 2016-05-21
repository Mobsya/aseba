import QtQuick 2.6

ListView {
	id: blockList

	property list<BlockDefinition> blocks
	property string backImage
	property bool isLandscape: true

	anchors.fill: parent
	clip: true
	orientation: isLandscape ? ListView.Vertical : ListView.Horizontal
	model: blocks

	delegate: MouseArea {
		width: blockList.isLandscape ? blockList.width : blockList.height + 16
		height: blockList.isLandscape ? blockList.width + 16 : blockList.height

		property Item highlightedBlock: null

		onClicked: {
			// only process in block editor mode
			if (blockEditor.visible) {
				blockEditor.setBlockType(blocks[index]);
			}
		}

		onPositionChanged: {
			// only process if not in block editor mode
			if (blockEditor.visible) {
				return;
			}

			var pos = mapToItem(mainContainer, mouse.x, mouse.y);
			blockDragPreview.x = pos.x - 128;
			blockDragPreview.y = pos.y - 128;
			if (!blockList.contains(mapToItem(blockList, mouse.x, mouse.y))) {
				blockDragPreview.backgroundImage = backImage;
				blockDragPreview.opacity = 0.8;
				blockDragPreview.params = blocks[index].defaultParams;
				blockDragPreview.definition = blocks[index];
			}
			var isHighlight = false;
			if (mainContainer.contains(pos)) {
				// find nearest block
				var scenePos = blockDragPreview.mapToItem(blockContainer, 128, 128);
				var minDist = Number.MAX_VALUE;
				var minIndex = 0;
				for (var i = 0; i < blockContainer.children.length; ++i) {
					var child = blockContainer.children[i];
					var destPos = child.mapToItem(blockContainer, 128, 128);
					var dx = scenePos.x - destPos.x;
					var dy = scenePos.y - destPos.y;
					var dist = Math.sqrt(dx*dx + dy*dy);
					if (dist < minDist) {
						minDist = dist;
						minIndex = i;
					}
				}

				// if blocks are close
				if (minDist < repulsionMaxDist) {
					var destBlock = blockContainer.children[minIndex];
					// highlight destblock
					if (highlightedBlock && highlightedBlock !== destBlock) {
						highlightedBlock.highlight = false;
					}
					destBlock.highlight = true;
					highlightedBlock = destBlock;
					isHighlight = true;
				}
			}
			// de we need to de-highlight?
			if (highlightedBlock && !isHighlight) {
				highlightedBlock.highlight = false;
				highlightedBlock = null;
			}
		}

		onReleased: {
			// only process if not in block editor mode
			if (blockEditor.visible) {
				return;
			}

			// create block
			var pos = mapToItem(mainContainer, mouse.x, mouse.y);
			if (mainContainer.contains(pos)) {
				var scenePos = mapToItem(scene, mouse.x, mouse.y);
				var newBlock = scene.createBlock(scenePos.x, scenePos.y, blockDragPreview.definition);

				// create link
				if (highlightedBlock) {
					newBlock.isStarting = false;
					var link = blockLinkComponent.createObject(linkContainer, {
						sourceBlock: highlightedBlock,
						destBlock: newBlock
					});
					// reset highlight
					highlightedBlock.highlight = false;
					highlightedBlock = null;
				}
			}

			// reset drop indicator
			blockDragPreview.definition = null;
		}

		HDPIImage {
			anchors.centerIn: parent
			source: backImage
			scale: blockList.width / 256
			width: 256 // working around Qt bug with SVG and HiDPI
			height: 256 // working around Qt bug with SVG and HiDPI
			Loader {
				enabled: false
				anchors.centerIn: parent
				sourceComponent: blocks[index].miniature
			}
		}
	}
}
