import QtQuick 2.5
import QtQml.Models 2.1

// editor
Rectangle {
	id: editor

	anchors.fill: parent

	color: "#1f000000"

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

	Item {
		id: placeholder
		anchors.centerIn: parent
	}

	onParamsChanged: {
		placeholder.children = []
		definition.editor.createObject(placeholder, {"params": params, "anchors.centerIn": placeholder})
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

	ListView {
		id: eventBlocksView
		model: eventBlocks
		anchors.left: parent.left
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		width: 256
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

	ListView {
		id: actionBlocksView
		model: actionBlocks
		anchors.right: parent.right
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		width: 256
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
				block.params = placeholder.children[0].getParams();
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
		placeholder.scale = size / 256;
	}
}
