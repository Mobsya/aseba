import QtQuick 2.7
import QtQuick.Controls.Material 2.2
import QtQuick.Layouts 1.3

Item {
	id: row

	property var astTransition: ({
		events: [event],
		actions: [],
		next: astState,
		trigger: function() {
			highlightTimer.highlight();
		}
	})
	property Item prev
	property Item next
	property int index: prev === null ? 0 : prev.index + 1

	width: layout.width + 2 * constants.rowPaddingH
	height: layout.height + 2 * constants.rowPaddingV

	Component.onCompleted: {
		rows.model.append(this);
		rows.updateWidth();
	}
	Component.onDestruction: {
		if (index < rows.model.count) {
			rows.model.remove(index, 1);
			rows.updateWidth();
		}
	}
	onWidthChanged: rows.updateWidth()

	function reserve() {
		if (next === null) {
			next = rows.append(this, null);
		}
		return next;
	}
	function free() {
		if (prev !== null) {
			prev.next = next;
		}
		if (next !== null) {
			next.prev = prev;
		}
		destroy();
	}
	function serialize() {
		var blocks = [];
		for (var block = event; block.next !== event; block = block.next) {
			var blockData = block.definition === null ? null : block.serialize(false);
			blocks.push(blockData);
		}
		return blocks;
	}
	function deserialize(data) {
		var block = event;
		data.forEach(function(blockData) {
			if (blockData !== null) {
				block.definition = definitionsByName[blockData.definition];
				block.params = blockData.params;
			}
			block = block.next;
		});
	}

	Rectangle {
		id: rowBackground
		anchors.fill: parent
		color: Material.theme === Material.Dark ? "#301446" : Material.background // "#ffead9"
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
		spacing: constants.actionSpacing

		Block {
			id: event

			property Block prev: event
			property Block next: event

			canDelete: definition !== null
			canGraph: false
			typeRestriction: "event"

			function free() {
				definition = null;
				params = undefined;
				if (next.definition === null) {
					row.free();
				}
			}

			onDefinitionChanged: {
				var index = astTransitions.indexOf(astTransition);
				var astIs = index !== -1;
				var astShould = definition !== null;
				if (astShould && !astIs) {
					astTransitions.push(astTransition);
					row.reserve();
				} else if (astIs && !astShould) {
					astTransitions.splice(index, 1);
				}
			}
			onParamsChanged: {
				astChanged();
			}
		}
		ColumnLayout {
			spacing: constants.actionSpacing
			Repeater {
				model: 2
				delegate: Rectangle {
					radius: 15
					color: Material.theme === Material.Dark ? "#9478aa" : "#72000000"
					height: radius
					width: radius
				}
			}
		}
		RowLayout {
			id: actions

			spacing: constants.actionSpacing

			Component.onCompleted: append(event)
			function append(prev) {
				var next = prev.next;
				var properties = {
					prev: prev,
					next: next,
				};
				var object = actionComponent.createObject(actions, properties);
				prev.next = object;
				next.prev = object;
				return object;
			}

			Component {
				id: actionComponent
				Block {
					id: action

					property Block prev
					property Block next

					canDelete: definition !== null
					canGraph: false
					typeRestriction: "action"

					function free() {
						console.assert(prev !== null);
						console.assert(next !== null);
						var index = astTransition.actions.indexOf(this);
						astTransition.actions.splice(index, 1);
						if (prev.definition === null && next.definition === null) {
							// there is just me and placeholders
							row.free();
						} else {
							prev.next = next;
							next.prev = prev;
							destroy();
						}
						astChanged();
					}

					onParamsChanged: {
						if (next === event) {
							// new action
							astTransition.actions.push(this);
							actions.append(this);
							row.reserve();
						}
						astChanged();
					}
				}
			}
		}
	}
}
