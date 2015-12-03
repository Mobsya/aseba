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

	visible: block !== null

	// to block events from going to the scene
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
		definition.editor.createObject(editorItemArea, {
			"params": params,
			"anchors.horizontalCenter": editorItemArea.horizontalCenter,
			"anchors.verticalCenter": editorItemArea.verticalCenter,
			"scale": Math.max(editorItemArea.width / 256, 0.1)
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
			model: eventBlocks
			anchors.fill: parent
			clip: true
			orientation: parent.isLandscape ? ListView.Vertical : ListView.Horizontal

			delegate: MouseArea {
				height: eventBlocksLoader.height
				width: eventBlocksLoader.width
				onClicked: {
					definition = eventBlocks[index];
					params = definition.defaultParams;
				}
				Rectangle {
					width: parent.width * 0.8
					height: parent.height * 0.8
					radius: parent.width / 2
					color: "#1c93d1"
					anchors.horizontalCenter: parent.horizontalCenter
					anchors.verticalCenter: parent.verticalCenter
					Loader {
						id: eventBlocksLoader
						enabled: false
						scale: 0.5
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
		anchors.rightMargin: isLandscape ? 50 : 0
		anchors.top: isLandscape ? parent.top : undefined
		anchors.left: isLandscape ? undefined : parent.left
		anchors.bottom: parent.bottom

		width: isLandscape ? 256 : undefined
		height: isLandscape ? undefined : 256
		color: "#ebeef0"

		ListView {
			id: actionBlocksView
			model: actionBlocks
			anchors.fill: parent
			clip: true
			orientation: parent.isLandscape ? ListView.Vertical : ListView.Horizontal

			delegate: MouseArea {
				height: actionBlocksLoader.height
				width: actionBlocksLoader.width
				onClicked: {
					definition = actionBlocks[index];
					params = definition.defaultParams;
				}
				Rectangle {
					width: parent.width * 0.8
					height: parent.height * 0.8
					radius: parent.width / 2
					color: "#f38420"
					anchors.horizontalCenter: parent.horizontalCenter
					anchors.verticalCenter: parent.verticalCenter
					Loader {
						id: actionBlocksLoader
						enabled: false
						scale: 0.5
						anchors.horizontalCenter: parent.horizontalCenter
						anchors.verticalCenter: parent.verticalCenter
						sourceComponent: actionBlocks[index].miniature
					}
				}
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

		onWidthChanged: {
			for (var i = 0; i < children.length; ++i) {
				children[i].scale = Math.max(width / 256, 0.1);
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
