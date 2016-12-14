import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import QtQuick.Window 2.2
import QtQml.Models 2.2

Item {
	id: scene

	property alias scale: rows.scale
	property var ast

	anchors.fill: parent
	onWidthChanged: rows.updateWidth()

	Component.onCompleted: clear()
	function clear() {
		ast = {
			blocks: [],
			links: [],
		};
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
		property int rowPaddingV: 10
		property int rowPaddingH: 40
		property int actionSpacing: 30
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
			Item {
				id: row

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
					color: Material.theme === Material.Dark ? "#301446" : "#ffead9"
					radius: height
				}
				RowLayout {
					id: layout

					x: constants.rowPaddingH
					y: constants.rowPaddingV
					spacing: constants.actionSpacing

					Item {
						width: event.startIndicator.width
					}
					Block {
						id: event

						property Block prev: event
						property Block next: event

						canDelete: definition !== null
						canGraph: false
						isStarting: true
						typeRestriction: "event"

						function free() {
							definition = null;
							params = undefined;
							if (next.definition === null) {
								row.free();
							}
						}

						onDefinitionChanged: {
							var index = scene.ast.blocks.indexOf(this);
							var astIs = index !== -1;
							var astShould = definition !== null;
							if (astShould && !astIs) {
								scene.ast.blocks.push(this);
								row.reserve();
							} else if (astIs && !astShould) {
								scene.ast.blocks.splice(index, 1);
							}
						}
						onParamsChanged: {
							astChanged();
						}

						LinkSimple {
							id: inbound
							sourceBlock: event.prev.prev
							destBlock: event
						}
					}
					ColumnLayout {
						spacing: constants.actionSpacing
						Repeater {
							model: 2
							delegate: Rectangle {
								radius: 15
								color: "#9478aa"
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
								isStarting: false
								typeRestriction: "action"

								function free() {
									console.assert(prev !== null);
									console.assert(next !== null);
									var index = scene.ast.blocks.indexOf(this);
									scene.ast.blocks.splice(index, 1);
									if (prev.definition === null && next.definition === null) {
										// there is just me and placeholders
										row.free();
									} else {
										prev.next = next;
										next.prev = prev;
										destroy();
										inbound.ast = false;
									}
									astChanged();
								}

								onParamsChanged: {
									if (next === event) {
										// new action
										scene.ast.blocks.push(this);
										actions.append(this);
										row.reserve();
									}
									astChanged();
								}

								LinkSimple {
									id: inbound
									sourceBlock: action.prev
									destBlock: action
								}
							}
						}
					}
				}
			}
		}
	}

	function handleBlockDrop(source, drop) {
		if (drop.source === blockDragPreview) {
			drop.accepted = true;
			var definition = blockDragPreview.definition;
			source.definition = definition;
			source.params = definition.defaultParams;
		} else {
			drop.accepted = false;
		}
	}

	function handleSceneDrop(source, drop) {
		drop.accepted = false;
	}
}
