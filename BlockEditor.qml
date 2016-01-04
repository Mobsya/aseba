import QtQuick 2.5
import QtQml.Models 2.1
import QtQuick.Window 2.2
import QtGraphicalEffects 1.0
import "blocks"

// editor
Rectangle {
	id: editor

	anchors.fill: parent

	//color: "#af000000"
	color: "#a9acaf"

//	Desaturate {
//		id: desaturated
//		anchors.fill: parent
//		source: mainContainer
//		desaturation: 0.5
//	}
//	FastBlur {
//	   anchors.fill: parent
//	   source: mainContainer
//	   radius: 32
//	}

	visible: block !== null

	// to block events from going to the scene
	PinchArea {
		anchors.fill: parent
	}
	MouseArea {
		anchors.fill: parent
		onWheel: wheel.accepted = true;
	}

	readonly property real blockListWidth: 220
	readonly property real blockListBlockScale: 0.72

	property list<BlockDefinition> eventBlocks: [
		ButtonsEventBlock {},
		ProxEventBlock {},
		ProxGroundEventBlock {},
		TapEventBlock {},
		ClapEventBlock {}
	]
	property list<BlockDefinition> actionBlocks: [
		MotorActionBlock {},
		TopColorActionBlock {},
		BottomColorActionBlock {}
	]

	property Block block: null
	property BlockDefinition definition: eventBlocks[0]
	property var params: definition.defaultParams

	onParamsChanged: {
		editorItemArea.children = []
		definition.editor.createObject(editorItemArea, {
			"params": params,
			"anchors.horizontalCenter": editorItemArea.horizontalCenter,
			"anchors.verticalCenter": editorItemArea.verticalCenter
		})
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
		id: editorItemArea

		property bool isLandscape: Window.width >= Window.height
		property real scaledWidth: isLandscape ? Math.min(Window.width - 512, Window.height - (96+20+20)*2) : Math.min(Window.width, Window.height - 512 - (96+20+20)*2)

		color: "transparent"

		anchors.centerIn: parent
		scale: Math.max(scaledWidth / 256, 0.1)
		width: 256
		height: 256
	}

	Rectangle {
		property bool isLandscape: Window.width >= Window.height

		anchors.left: parent.left
		anchors.top: parent.top
		anchors.bottom: isLandscape ? parent.bottom : undefined
		anchors.right: isLandscape ? undefined : parent.right

		width: isLandscape ? blockListWidth : undefined
		height: isLandscape ? undefined : blockListWidth
		color: "#ebeef0"

		ListView {
			id: eventBlocksView
			model: eventBlocks
			anchors.fill: parent
			clip: true
			orientation: parent.isLandscape ? ListView.Vertical : ListView.Horizontal

			delegate: MouseArea {
				height: blockListWidth
				width: blockListWidth
				onClicked: {
					definition = eventBlocks[index];
					params = definition.defaultParams;
				}
				Image {
					anchors.centerIn: parent
					source: "images/eventCenter.svg"
					scale: blockListBlockScale
					Loader {
						id: eventBlocksLoader
						enabled: false
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
						sourceComponent: eventBlocks[index].miniature
					}
				}
			}
		}
	}

	Rectangle {
		property bool isLandscape: Window.width >= Window.height

		anchors.right: parent.right
		anchors.top: isLandscape ? parent.top : undefined
		anchors.left: isLandscape ? undefined : parent.left
		anchors.bottom: parent.bottom

		width: isLandscape ? blockListWidth : undefined
		height: isLandscape ? undefined : blockListWidth
		color: "#ebeef0"

		ListView {
			id: actionBlocksView
			model: actionBlocks
			anchors.fill: parent
			clip: true
			orientation: parent.isLandscape ? ListView.Vertical : ListView.Horizontal

			delegate: MouseArea {
				height: blockListWidth
				width: blockListWidth
				onClicked: {
					definition = actionBlocks[index];
					params = definition.defaultParams;
				}
				Image {
					anchors.centerIn: parent
					source: "images/actionCenter.svg"
					scale: blockListBlockScale
					Loader {
						id: actionBlocksLoader
						enabled: false
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
						sourceComponent: actionBlocks[index].miniature
					}
				}
			}
		}

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
				block.definition = definition;
				block.params = editorItemArea.children[0].getParams();
				block = null;
			}
		}
	}
}
