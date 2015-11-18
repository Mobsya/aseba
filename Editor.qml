import QtQuick 2.0
import QtQml.Models 2.1

// editor
Rectangle {
	id: editor

	anchors.fill: parent

	color: "#1f000000"

	visible: false

	property Item editorItem: null
	property Item editedBlock: null

	function setEditorItem(item) {
		if (editorItem) {
			editorItem.parent = null;
			editorItem = null;
		}
		editorItem = item;
		editorItem.anchors.horizontalCenter = editor.horizontalCenter;
		editorItem.anchors.verticalCenter = editor.verticalCenter;
		resizeEditor();
	}

	// to blok events from going to the scene
	PinchArea {
		anchors.fill: parent
	}
	MouseArea {
		anchors.fill: parent
		onWheel: wheel.accepted = true;
	}

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
			setEditorItem(currentItem.createEditor(parent));
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
