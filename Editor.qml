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

	visible: block !== null

	// to blok events from going to the scene
	PinchArea {
		anchors.fill: parent
	}
	MouseArea {
		anchors.fill: parent
		onWheel: wheel.accepted = true;
	}

	property list<BlockDefinition> eventBlocks: [
		ProxEventBlock {},
		ProxEventBlock {},
		ProxEventBlock {}
	]
	property list<BlockDefinition> actionBlocks: [
		MotorActionBlock {},
		MotorActionBlock {},
		MotorActionBlock {}
	]

	property Block block: null
	property BlockDefinition definition: eventBlocks[0]
	property var params: definition.defaultParams

	onParamsChanged: {
		editorItemArea.children = []
		definition.editor.createObject(editorItemArea, {"params": params})
		resizeEditor();
	}

	onBlockChanged: {
		if (block) {
			definition = block.definition;
			params = block.params;
		} else {
			params = definition.defaultParams;
		}
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
			model: eventBlocks
			anchors.fill: parent
			clip: true

			delegate: MouseArea {
				height: eventBlocksLoader.height
				width: eventBlocksLoader.width
				onClicked: {
					definition = eventBlocks[index];
					params = definition.defaultParams;
				}
				Loader {
					id: eventBlocksLoader
					enabled: false
					sourceComponent: eventBlocks[index].miniature
				}
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
			model: actionBlocks
			anchors.fill: parent
			clip: true

			delegate: MouseArea {
				height: actionBlocksLoader.height
				width: actionBlocksLoader.width
				onClicked: {
					definition = actionBlocks[index];
					params = definition.defaultParams;
				}
				Loader {
					id: actionBlocksLoader
					enabled: false
					sourceComponent: actionBlocks[index].miniature
				}
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
				block.definition = definition;
				block.params = editorItemArea.children[0].getParams();
				block = null;
			}
		}
	}

	onHeightChanged: {
		resizeEditor();
	}

	function resizeEditor() {
		var availableHeight = height - 20*8 - closeButton.height;
		var availableWidth = width - actionBlocksView.width - eventBlocksView.width;
		var size = Math.min(availableWidth, availableHeight);
		editorItemArea.scale = size / 256;
	}
}
