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
	property var astState: ({
		transitions: [],
		active: false,
	});
	property var ast
	function updateAst() {
		var astTransitions = [];
		for (var i = 0; i < rows.model.count - 1; ++i) {
			var row = rows.model.get(i);
			astTransitions.push(row.content.ast);
		}
		astState.transitions = astTransitions;
		ast = [ astState ];
	}

	anchors.fill: parent
	onWidthChanged: rows.updateWidth()

	Component.onCompleted: clear()
	function clear() {
		rows.model.clear();
		rows.appendEmpty();
		scene.updateAst();
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
		rows.model.clear();
		data.forEach(function(data) {
			rows.append(data);
		});
		rows.appendEmpty();
		scene.updateAst();
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

		property int listWidth
		property int leftOffset: scene.width - listWidth * scale
		property int maxEventWidth: 0

		height: parent.height / scale
		width: parent.width / scale
		leftMargin: constants.rowSpacing
		rightMargin: constants.rowSpacing
		scale: Math.min(0.5, scene.width / listWidth)
		topMargin: Math.max(constants.rowSpacing, (height - contentHeight) / 2)
		bottomMargin: topMargin

		spacing: constants.rowSpacing

		model: ObjectModel {
		}

		function append(data) {
			var row = rowComponent.createObject(rows.model);
			if (data !== undefined) {
				row.content.deserialize(data);
			}
			rows.model.append(row);
			rows.updateMaxEventWidth();
			rows.updateWidth();
		}
		function appendEmpty() {
			append(undefined);
		}

		function remove(row) {
			rows.model.remove(row.ObjectModel.index);
			rows.updateMaxEventWidth();
			rows.updateWidth();
		}

		function updateWidth() {
			var width = 0;
			for (var i = 0; i < model.count; ++i) {
				var row = model.get(i);
				width = Math.max(width, maxEventWidth - row.content.eventWidth + row.width);
			}
			listWidth = leftMargin + width + rightMargin;
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
				anchors.leftMargin: rows.leftOffset + rows.maxEventWidth - content.eventWidth
				width: content.width
				height: content.height
				onWidthChanged: rows.updateWidth()

				property bool held: false
				onPressAndHold: if (!isLast()) held = true
				onReleased: {
					if (eventPane.trashOpen || actionPane.trashOpen) {
						rows.remove(row);
						scene.updateAst();
					}
					held = false;
				}

				drag.target: held ? content : undefined

				function isLast() {
					return row.ObjectModel.index === rows.model.count - 1;
				}

				onPositionChanged: {
					// check whether we are hovering delete block item
					eventPane.trashOpen = eventPane.contains(mapToItem(eventPane, mouse.x, mouse.y));
					actionPane.trashOpen = actionPane.contains(mapToItem(actionPane, mouse.x, mouse.y));
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
					Drag.hotSpot.x: width / 2
					Drag.hotSpot.y: height / 2
					Drag.keys: ["row"]
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
						if (row.ObjectModel.index === -1)
							return;
						var last = row.isLast();
						var empty = ast.events.length === 0 && ast.actions.length === 0;
						if (last && !empty) {
							rows.appendEmpty();
						} else if (empty && !last) {
							rows.remove(row);
						}
						rows.positionViewAtIndex(row.ObjectModel.index, ListView.Contain);
						scene.updateAst();
					}
				}

				DropArea {
					anchors { fill: parent; }
					keys: ["row"]

					onEntered: {
						if (row.isLast()) {
							drag.accepted = false;
							return;
						}
						rows.model.move(drag.source.ObjectModel.index, row.ObjectModel.index);
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
