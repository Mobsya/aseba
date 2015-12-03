import QtQuick 2.5
import QtQml.Models 2.1
import QtQuick.Window 2.2
import QtGraphicalEffects 1.0

// editor
Rectangle {
	id: editor

	anchors.fill: parent

	color: "#7f000000"

	Desaturate {
		anchors.fill: parent
		source: mainContainer
		desaturation: 0.5
	}

	visible: false

	property Item editedBlock: null
	property Item editorItem: null

	function setEditorItem(item) {
		if (editorItem) {
			editorItem.parent = null;
			editorItem = null;
		}
		editorItem = item;
		editorItem.parent = editorItemArea;
		editorItem.anchors.horizontalCenter = editorItemArea.horizontalCenter;
		editorItem.anchors.verticalCenter = editorItemArea.verticalCenter;
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
		ProxEventBlock { enabled: false; }
	}

	ObjectModel {
		id: actionBlocksModel
		MotorActionBlock { enabled: false }
	}

	Rectangle {
		anchors.left: parent.left
		anchors.leftMargin: Screen.width > Screen.height ? 50 : 0
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		width: 256
		color: "white";

		ListView {
			id: eventBlocksView
			model: eventBlocksModel
			anchors.fill: parent
			clip: true

			delegate: Component {
				Item {
					anchors.fill: parent
					MouseArea {
						anchors.fill: parent
						onClicked: { eventBlocksView.currentIndex = index; console.log(index); }
					}
				}
			}

			onCurrentItemChanged: {
				console.log("event " + currentItem);
				actionBlocksView.currentIndex = -1;
				// re-create a new editor when another block is selected
				setEditorItem(currentItem.createEditor(parent));
			}
		}
	}

	Rectangle {
		anchors.right: parent.right
		anchors.rightMargin: Screen.width > Screen.height ? 50 : 0
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		width: 256
		color: "white";

		ListView {
			id: actionBlocksView
			model: actionBlocksModel
			anchors.fill: parent
			clip: true

			onCurrentItemChanged: {
				console.log("action " + currentItem);
				eventBlocksView.currentIndex = -1;
				// re-create a new editor when another block is selected
				setEditorItem(currentItem.createEditor(parent));
			}
		}
	}

	Rectangle {
		id: editorItemArea

		color: "gray"
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.verticalCenter: parent.verticalCenter

		width: childrenRect.width
		height: childrenRect.height
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
