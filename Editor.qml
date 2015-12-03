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
		id: desaturated
		anchors.fill: parent
		source: mainContainer
		desaturation: 0.5
	}
//	FastBlur {
//	   anchors.fill: parent
//	   source: mainContainer
//	   radius: 32
//	}

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
		property bool isLandscape: Window.width >= Window.height

		anchors.left: parent.left
		anchors.leftMargin: isLandscape ? 50 : 0
		anchors.top: parent.top
		anchors.bottom: isLandscape ? parent.bottom : undefined
		anchors.right: isLandscape ? undefined : parent.right

		width: isLandscape ? 256 : undefined
		height: isLandscape ? undefined : 256
		color: "#ebeef0"

		ListView {
			id: eventBlocksView
			model: eventBlocksModel
			anchors.fill: parent
			clip: true
			orientation: parent.isLandscape ? ListView.Vertical : ListView.Horizontal

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
		property bool isLandscape: Window.width >= Window.height

		anchors.right: parent.right
		anchors.rightMargin: isLandscape ? 50 : 0
		anchors.top: isLandscape ? parent.top : undefined
		anchors.left: isLandscape ? undefined : parent.left
		anchors.bottom: parent.bottom

		width: isLandscape ? 256 : undefined
		height: isLandscape ? undefined : 256
		color: "#ebeef0"

		ListView {
			id: actionBlocksView
			model: actionBlocksModel
			anchors.fill: parent
			clip: true
			orientation: parent.isLandscape ? ListView.Vertical : ListView.Horizontal

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

		property bool isLandscape: Window.width >= Window.height

		color: "#a9acaf"
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.verticalCenter: parent.verticalCenter

		width: isLandscape ? Math.min(Window.width - 512 - 100, Window.height - (96+20+20)*2) : Math.min(Window.height, Window.height - 512 - (96+20+20)*2)
		height: width
	}

	Image {
		id: closeButton

		property bool isLandscape: Window.width >= Window.height

		source: "images/okButton.svg"
		width: 96
		height: 96

		anchors.bottom: parent.bottom
		anchors.bottomMargin: isLandscape ? 20 : 256 + 20
		anchors.horizontalCenter: parent.horizontalCenter

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
			editorItem.scale = editorItemArea.width / 256;
		}
	}
}
