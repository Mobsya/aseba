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

		onClicked: {
			// only process in block editor mode
			if (blockEditor.visible) {
				blockEditor.setBlockType(blocks[index]);
			}
		}

		// we start the drag when the cursor is outside the area,
		// otherwise the list scroll does not work
		onPositionChanged: {
			// only process if not in block editor mode
			if (blockEditor.visible) {
				return;
			}

			if (this.drag.target !== null) {
				// already dragging
				return;
			}
			var pos = mapToItem(mainContainer, mouse.x, mouse.y);
			if (!mainContainer.contains(pos)) {
				// not inside main container
				return;
			}

			var definition = blocks[index];
			blockDragPreview.x = pos.x - blockDragPreview.width / 2;
			blockDragPreview.y = pos.y - blockDragPreview.height / 2;
			blockDragPreview.params = definition.defaultParams;
			blockDragPreview.definition = definition;
			blockDragPreview.Drag.active = true;
			this.drag.target = blockDragPreview;
		}

		onReleased: {
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
				sourceComponent: blocks[index].miniature
			}
		}
	}
}
