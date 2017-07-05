import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Window 2.2
import QtQml.Models 2.2

Item {
	id: scene

	property alias scale: rows.scale

	/*
		In simple mode, there is only one thread with only one state.
		Each transition (row in the UI) goes from this only state to that same state.
	*/
	property var astTransitions
	property var astState
	property var ast

	anchors.fill: parent
	onWidthChanged: rows.updateWidth()

	Component.onCompleted: clear()
	function clear() {
		astTransitions = [];
		astState = {
			transitions: astTransitions,
			active: false,
		};
		ast = [ astState ];
		rows.model.clear();
		rows.append(null, null);
	}

	function serialize() {
		var data = [];
		for (var i = 0; i < rows.model.count - 1; ++i) {
			data.push(rows.model.get(i).serialize());
		}
		return data;
	}

	function deserialize(data) {
		clear();
		var row = rows.model.get(0);
		data.forEach(function(data) {
			row.deserialize(data);
			row = row.next;
		});
	}

	function deleteBlock(block) {
		block.free();
	}

	QtObject {
		id: constants
		property int rowSpacing: 60
	}

	ListView {
		id: rows

		anchors.centerIn: parent

		leftMargin: constants.rowSpacing
		width: leftMargin + contentWidth + rightMargin
		rightMargin: constants.rowSpacing
		scale: Math.min(0.5, scene.width / width)

		height: parent.height / scale
		topMargin: Math.max(constants.rowSpacing, (height - contentHeight) / 2)
		bottomMargin: topMargin

		spacing: constants.rowSpacing

		model: ObjectModel {
		}

		function append(prev, next) {
			var properties = {
				prev: prev,
				next: next,
			};
			var object = rowComponent.createObject(rows, properties);
			return object;
		}

		function updateWidth() {
			var width = 0;
			for (var i = 0; i < model.count; ++i) {
				var row = model.get(i);
				width = Math.max(width, row.width);
			}
			contentWidth = width;
		}

		Component {
			id: rowComponent
			StateTransition {
			}
		}
	}

	function handleBlockDrop(source, drop) {
		if (drop.source === blockDragPreview) {
			drop.accepted = true;
			var definition = blockDragPreview.definition;
			source.definition = definition;
			source.params = definition.defaultParams;
			blockEditor.setBlock(source);
		} else {
			drop.accepted = false;
		}
	}

	function handleSceneDrop(source, drop) {
		drop.accepted = false;
	}
}
