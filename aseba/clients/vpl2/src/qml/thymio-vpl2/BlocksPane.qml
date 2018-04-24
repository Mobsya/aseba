import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2

Pane {
	id: pane

	property list<BlockDefinition> blocks
	property string backImage
	property bool showTrash: false
	property bool trashOpen: false
	property bool isMini

	width: isMini ? 64 : 96
	height: parent.height
	Material.elevation: 6

	BlocksList {
		id: blocksList
		blocks: pane.blocks
		backImage: pane.backImage
		anchors.fill: parent
	}

	Rectangle {
		anchors.fill: parent
		color: background.color
		opacity: pane.showTrash ? 0.8 : 0.0
		Behavior on opacity { PropertyAnimation {} }
	}

	HDPIImage {
		source: trashOpen ?
					( Material.theme === Material.Dark ? "icons/ic_delete_on_white_48px.svg" : "icons/ic_delete_on_black_48px.svg" ) :
					( Material.theme === Material.Dark ? "icons/ic_delete_white_48px.svg" : "icons/ic_delete_black_48px.svg" )
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
