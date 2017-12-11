import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Window 2.2
import QtQml.Models 2.2

Item {
	id: scene

	property alias scale: rows.scale
	property int eventCountMax: 5
	property int actionCountMax: 5

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
			var row = rows.model.get(i);
			data.push(row.content.serialize());
		}
		return data;
	}

	function deserialize(data) {
		clear();
		var row = rows.model.get(0);
		data.forEach(function(data) {
			row.content.deserialize(data);
			row = row.next;
		});
	}

	function deleteBlock(block) {
		block.destroy();
	}

	QtObject {
		id: constants
		property int rowSpacing: 60
	}

	ListView {
		id: rows

		anchors.centerIn: parent

		property int maxEventWidth: 0

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
				width = Math.max(width, row.width + row.leftMargin);
			}
			contentWidth = width;
		}

		function updateMaxEventWidth() {
			var newMaxEventWidth = 0;
			for (var i = 0; i < model.count; ++i) {
				var row = model.get(i);
				newMaxEventWidth = Math.max(newMaxEventWidth, row.content.eventWidth);
			}
			maxEventWidth = newMaxEventWidth;
		}

		Component {
			id: rowComponent
			MouseArea {
				id: row
				anchors.left: parent ? parent.left : undefined
				property int leftMargin: anchors.leftMargin
				anchors.leftMargin: rows.maxEventWidth - content.eventWidth
				width: content.width
				height: content.height
				onWidthChanged:	rows.updateWidth()

				property Item prev
				property Item next
				property int index: prev === null ? 0 : prev.index + 1

				property bool held: false
				onPressAndHold: if (next !== null) held = true
				onReleased: {
					if (eventPane.trashOpen || actionPane.trashOpen) {
						row.remove();
						scene.astChanged();
					}
					held = false;
				}

				drag.target: held ? content : undefined

				Component.onCompleted: {
					rows.model.append(this);
					rows.updateMaxEventWidth();
					rows.updateWidth();
				}
				Component.onDestruction: {
					if (index < rows.model.count) {
						rows.model.remove(index, 1);
						rows.updateMaxEventWidth();
						rows.updateWidth();
					}
				}

				onPositionChanged: {
					// check whether we are hovering delete block item
					eventPane.trashOpen = eventPane.contains(mapToItem(eventPane, mouse.x, mouse.y));
					actionPane.trashOpen = actionPane.contains(mapToItem(actionPane, mouse.x, mouse.y));
				}

				function remove() {
					if (prev !== null) {
						prev.next = next;
					}
					next.prev = prev;
					destroy();
					astTransitions.splice(index, 1);
				}

				property TransitionRow content: TransitionRow {
					id: content
					nextState: astState
					eventCountMax: scene.eventCountMax
					actionCountMax: scene.actionCountMax

					parent: row
					anchors {
						horizontalCenter: parent.horizontalCenter
						verticalCenter: parent.verticalCenter
					}
					onEventWidthChanged: rows.updateMaxEventWidth()

					Drag.source: row
					Drag.active: row.held
					states: State {
						when: row.held
						ParentChange {
							target: content
							parent: scene
						}
						AnchorChanges {
							target: content
							anchors {
								horizontalCenter: undefined
								verticalCenter: undefined
							}
						}
						PropertyChanges {
							target: content
							highlighted: true
						}
						PropertyChanges {
							target: rows
							interactive: false
						}
						PropertyChanges {
							target: eventPane
							showTrash: true
						}
						PropertyChanges {
							target: actionPane
							showTrash: true
						}
					}

					onAstChanged: {
						var last = next === null;
						var empty = ast.events.length === 0 && ast.actions.length === 0;
						if (last && !empty) {
							next = rows.append(row, null);
							astTransitions.splice(index, 0, ast);
						} else if (empty && !last) {
							remove();
						}
						rows.positionViewAtIndex(index, ListView.Contain);
						scene.astChanged();
					}
				}
			}
		}
	}

	function handleBlockDrop(source, drop) {
		if (drop.source === blockDragPreview) {
			drop.accepted = true;
			var definition = blockDragPreview.definition;
			var params = definition.defaultParams;
			blockEditor.openBlock(source, definition, params);
		} else {
			drop.accepted = false;
		}
	}

	function handleSceneDrop(source, drop) {
		drop.accepted = false;
	}
}
