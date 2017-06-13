import QtQuick 2.7

ListView {
	id: blockList

	property list<BlockDefinition> blocks
	property string backImage
	property bool isLandscape: true

	// scrolling is implemented by hand
	// otherwise, it prevents block drag and drop from working
	interactive: false

	anchors.fill: parent
	//clip: true
	orientation: isLandscape ? ListView.Vertical : ListView.Horizontal
	model: blocks

	delegate: MouseArea {
		property BlockDefinition definition: blocks[index]

		width: blockList.isLandscape ? blockList.width : blockList.height + 16
		height: blockList.isLandscape ? blockList.width + 16 : blockList.height

		property int startPos
		property bool moved
		onPressed: {
			startPos = blockList.isLandscape ? y + mouse.y : x + mouse.x;
			moved = false;
		}

		onClicked: {
			if (moved) {
				return;
			}

			// only process in block editor mode
			if (blockEditor.visible) {
				blockEditor.setBlockType(definition);
			}
		}

		// we start the drag when the cursor is outside the area,
		// otherwise the list scroll does not work
		onPositionChanged: {
			if (this.drag.target !== null) {
				// already dragging
				return;
			}

			var blockListPos = mapToItem(blockList, mouse.x, mouse.y);
			if (blockList.contains(blockListPos)) {
				moved = true;
				if (blockList.isLandscape) {
					if (blockList.contentItem.height > blockList.height) {
						blockList.contentY = startPos - blockListPos.y;
					}
				} else {
					if (blockList.contentItem.width > blockList.width) {
						blockList.contentX = startPos - blockListPos.x;
					}
				}
			}

			if (!blockEditor.visible) {
				// only process if not in block editor mode
				var mainContainerPos = mapToItem(mainContainer, mouse.x, mouse.y);
				if (mainContainer.contains(mainContainerPos)) {
					blockDragPreview.x = mainContainerPos.x - blockDragPreview.width / 2;
					blockDragPreview.y = mainContainerPos.y - blockDragPreview.height / 2;
					blockDragPreview.params = definition.defaultParams;
					blockDragPreview.definition = definition;
					blockDragPreview.Drag.active = true;
					this.drag.target = blockDragPreview;
				}
			}
		}

		onExited: {
			blockList.returnToBounds();
		}

		onReleased: {
			blockList.returnToBounds();

			if (this.drag.target === null) {
				// not dragging
				return;
			}

			blockDragPreview.Drag.drop();
			this.drag.target = null;
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
				sourceComponent: definition.miniature
			}
		}
	}
}
