import QtQuick 2.7
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3

Item {
	id: row
	property int eventCountMax: 5
	property int actionCountMax: 5

	property var nextState
	property var ast: ({
		events: events.ast,
		actions: actions.ast,
		next: nextState,
		trigger: function() {
			highlightTimer.highlight();
		}
	})

	QtObject {
		id: constants
		property int rowPaddingV: 8
		property int rowPaddingH: 14
		property int blockSpacing: 0
		property int columnSignRadius: 9
		property int columnSignSpacing: 48
	}

	width: layout.width + 2 * constants.rowPaddingH
	height: layout.height + 2 * constants.rowPaddingV

	function serialize() {
		return [events, actions].map(function(container) {
			var blocks = container.children;
			var blocksData = [];
			for (var i = 0; i < blocks.length; ++i) {
				var block = blocks[i];
				var blockData = block.serialize(false);
				blocksData.push(blockData);
			}
			return blocksData;
		});
	}
	function deserialize(data) {
		var containers = [events, actions];
		for (var i = 0; i < containers.length; ++i) {
			var container = containers[i];
			var containerData = data[i];
			containerData.forEach(function(blockData) {
				var definition = definitionsByName[blockData.definition];
				var params = blockData.params;
				container.append(definition, params);
			});
		}
	}

	Rectangle {
		id: rowBackground
		anchors.fill: parent
		color: Material.theme === Material.Dark ? "#301446" : "#eaeaea"
		radius: height
		border.color: "#F5E800"
		border.width: highlightTimer.highlighted ? 10 : 0
		HighlightTimer {
			id: highlightTimer
		}
	}
	RowLayout {
		id: layout

		x: constants.rowPaddingH
		y: constants.rowPaddingV
		spacing: constants.blockSpacing

		TransitionBlocks {
			id: events
			typeRestriction: "event"
		}
		Block {
			visible: eventCountMax === -1 || eventCountMax > events.ast.length
			canDelete: false
			typeRestriction: "event"
			onParamsChanged: {
				if (definition === null) {
					// we just got reset
					return;
				}
				events.append(definition, params);
				definition = null;
				params = undefined;
			}
		}
		ColumnLayout {
			spacing: constants.columnSignSpacing
			Repeater {
				model: 2
				delegate: Rectangle {
					radius: constants.columnSignRadius
					color: Material.theme === Material.Dark ? "#9478aa" : "#cccccc"
					height: radius * 2
					width: radius * 2
				}
			}
		}
		TransitionBlocks {
			id: actions
			typeRestriction: "action"
		}
		Block {
			visible: actionCountMax === -1 || actionCountMax > actions.ast.length
			canDelete: false
			typeRestriction: "action"
			onParamsChanged: {
				if (definition === null) {
					// we just got reset
					return;
				}
				actions.append(definition, params);
				definition = null;
				params = undefined;
			}
		}
	}
}
