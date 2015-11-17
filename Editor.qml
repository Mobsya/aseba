import QtQuick 2.0
import QtQml.Models 2.1

// editor
Rectangle {
	id: editor

	width: screen.width - 200
	height: screen.height - 200
	anchors.horizontalCenter: parent.horizontalCenter
	anchors.verticalCenter: parent.verticalCenter

	color: "#3f000000"

	visible: false

	property Item editorItem: null
	property Item editedBlock: null

	ObjectModel {
		id: eventBlocksModel
		ProxEventBlock { }
	}

	ObjectModel {
		id: actionBlocksModel
		/*ProxEventBlock { }
		ProxEventBlock { }
		ProxEventBlock { }
		ProxEventBlock { }
		ProxEventBlock { }
		ProxEventBlock { }*/
	}

	ListView {
		id: eventBlocksView
		model: eventBlocksModel
		anchors.left: parent.left
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		width: 256
		clip: true

		onCurrentItemChanged: {
			console.log("event " + currentItem);
			// re-create a new editor when another block is selected
			editorItem = currentItem.createEditor(parent);
			editorItem.anchors.horizontalCenter = parent.horizontalCenter;
			editorItem.anchors.verticalCenter = parent.verticalCenter;
			//parent.resizeEditor();
		}
	}

	ListView {
		id: actionBlocksView
		model: actionBlocksModel
		anchors.right: parent.right
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		width: 256
		clip: true

		onCurrentItemChanged: {
			console.log("action " + currentItem);
		}
	}

	Rectangle {
		id: closeButton

		width: 200
		height: 50

		anchors.bottom: parent.bottom
		anchors.bottomMargin: 20
		anchors.horizontalCenter: parent.horizontalCenter

		radius: 50
		color: "black"

		MouseArea {
			anchors.fill: parent
			onClicked: {
				editedBlock.type = editorItem.getType();
				editedBlock.name = editorItem.getName();
				editedBlock.params = editorItem.getParams();
				editedBlock.miniature = editorItem.getMiniature();
				editedBlock.miniature.parent = editedBlock;
				editedBlock = null;
				editor.visible = false;
			}
		}
	}

	onHeightChanged: {
		resizeEditor();
	}

	function resizeEditor() {
		if (editorItem) {
			var availableHeight = height - 20*8 - closeButton.height;
			var availableWidth = width - actionBlocksView.width - eventBlocksView.width;
			var size = Math.min(availableWidth, availableHeight);
			editorItem.scale = size / 256;
		}
	}
}
