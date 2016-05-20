import QtQuick 2.6
import Qt.labs.controls 1.0

Pane {
	id: pane

	property list<BlockDefinition> blocks
	property string backImage
	property bool showTrash: false
	property bool trashOpen: false

	width: 96
	height: parent.height

	background: Rectangle {
		color: "#301446"
	}

	BlocksList {
		blocks: pane.blocks
		backImage: pane.backImage
		anchors.fill: parent
	}

	Rectangle {
		anchors.fill: parent
		color: "#301446"
		opacity: pane.showTrash ? 0.8 : 0.0
		Behavior on opacity { PropertyAnimation {} }
	}

	HDPIImage {
		source: trashOpen ? "images/ic_delete_on_white_48px.svg" : "images/ic_delete_white_48px.svg"
		width: 64 // working around Qt bug with SVG and HiDPI
		height: 64 // working around Qt bug with SVG and HiDPI
		anchors.centerIn: parent
		opacity: pane.showTrash ? 1.0 : 0.0

		Behavior on opacity { PropertyAnimation {} }
	}

	function clearTrash() {
		showTrash = false;
		trashOpen = false;
	}
}
