import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import QtQuick.Window 2.2
import QtQml.Models 2.2

Item {
	id: scene
	readonly property rect viewRect: childrenRect

	property var ast: ({
		blocks: [],
		links: [],
	})

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

	ColumnLayout {
		id: rows

		spacing: constants.rowSpacing

		Component.onCompleted: {
			append(null, null);
		}
		function append(prev, next) {
			var properties = {
				prev: prev,
				next: next,
			};
			return rowComponent.createObject(rows, properties);
		}

		Component {
			id: rowComponent
			Item {
				id: row

				property Item prev
				property Item next

				implicitWidth: layout.width + 2 * constants.rowPaddingH
				implicitHeight: layout.height + 2 * constants.rowPaddingV

				function reserve() {
					if (row.next === null) {
						row.next = rows.append(row, null);
					}
				}
				function free() {
					if (prev !== null) {
						prev.next = next;
					}
					if (next !== null) {
						next.prev = prev;
					}
					astChanged();
					destroy();
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
							if (next.definition === null) {
								row.free();
							} else {
								definition = null;
								params = undefined;
							}
						}

						onDefinitionChanged: {
							var index = scene.ast.blocks.indexOf(this);
							var astIs = index !== -1;
							var astShould = definition !== null;
							if (astIs != astShould) {
								if (astShould) {
									scene.ast.blocks.push(this);
									row.reserve();
								} else {
									scene.ast.blocks.splice(index, 1);
								}
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

						Component.onCompleted: append(event, event)
						function append(prev, next) {
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
									if (prev.definition === null && next.definition === null) {
										// there is just me and placeholders
										row.free();
									} else {
										prev.next = next;
										next.prev = prev;
										var index = scene.ast.blocks.indexOf(this);
										scene.ast.blocks.splice(index, 1);
										destroy();
										inbound.ast = false;
										astChanged();
									}
								}

								onParamsChanged: {
									if (next === event) {
										// new action
										scene.ast.blocks.push(this);
										actions.append(this, next);
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
}
