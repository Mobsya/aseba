import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQml.Models 2.1
import QtQuick.Window 2.2
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import "blocks"

// editor
Item {
	id: editor

	visible: false

	Rectangle {
		anchors.fill: parent
		color: Material.theme === Material.Dark ? "#44285a" : "#a0b2b3"
		//opacity: 0.8
	}

	// FIXME: still a problem?
	// to block events from going to the scene
	PinchArea {
		anchors.fill: parent
	}
	MouseArea {
		anchors.fill: parent
		onWheel: wheel.accepted = true;
	}

	property string typeRestriction // "", "event", "action"
	property var callback // function called when done
	property var params
	property BlockDefinition definition

	function open(typeRestriction, params, definition, callback) {
		editor.typeRestriction = typeRestriction;
		editor.params = params;
		editor.definition = definition;
		editor.callback = callback;
		editor.visible = true;
	}

	function setBlock(block) {
		var typeRestriction = block.typeRestriction;
		var params = block.params;
		var definition = block.definition;
		open(typeRestriction, params, definition, function(definition, params) {
			block.definition = definition;
			block.params = params;
		});
	}

	function setBlockType(definition) {
		if (typeRestriction !== "" && typeRestriction !== definition.type) {
			return;
		}
		editor.params = definition.defaultParams;
		editor.definition = definition;
	}

	function close() {
		editor.visible = false;
		editor.definition = null;
		editor.params = undefined;
		editor.callback = undefined;
		editor.typeRestriction = "";
	}

	function accept() {
		if (definition === null) {
			return;
		}
		var params = editorItemLoader.item.getParams();
		callback(definition, params);
		close();
	}

	Loader {
		id: editorItemLoader
		anchors.centerIn: parent
		scale: Math.max(scaledWidth / 256, 0.1)

		//property real scaledWidth: Math.min(parent.width, parent.height - (cancelButton.height+12+12)*2)
		property real scaledWidth: Math.min(parent.width-24, parent.height-24)

		sourceComponent: definition !== null ? definition.editor : null
		onLoaded: item.params = params
	}
}
