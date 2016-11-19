import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import QtQuick.Window 2.2
import QtQml.Models 2.2

Item {
	readonly property rect viewRect: childrenRect

	property var ast: ({
		blocks: [],
		links: [],
	})
	function recompile() {
		astChanged();
	}

	function deleteBlock(block) {
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
				function createLink(sourceBlock, destBlock) {
					var properties = {
						sourceBlock: sourceBlock,
						destBlock: destBlock,
					};
					var link = linkComponent.createObject(sourceBlock, properties);
					sourceBlock.outbound = link;
					return link;
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

						property Link outbound

						canDelete: definition !== null
						canGraph: false
						isStarting: true
						typeRestriction: "event"
						onParamsChanged: {
							var firstAction = actions.children[0];
							if (firstAction.definition !== null) {
								// row is valid now
								if (event.outbound === null) {
									// row was not valid previously
									ast.blocks.push(event);
									ast.links.push(createLink(event, firstAction));
									for (var i = 0; i < actions.children.length - 1; ++i) {
										var action = actions.children[i];
										ast.blocks.push(action);
										var link = action.outbound;
										if (link === null) {
											link = createLink(action, event);
										}
										ast.links.push(link);
									}
								}
								recompile();
							}
							row.reserve();
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

						Component.onCompleted: append(null, null)
						function append(prev, next) {
							var properties = {
								prev: prev,
								next: next,
							};
							return actionComponent.createObject(actions, properties);
						}

						Component {
							id: actionComponent
							Block {
								id: action

								property Block prev
								property Block next
								property Link outbound

								canDelete: definition !== null
								canGraph: false
								isStarting: false
								typeRestriction: "action"

								function reserve() {
									action.next = actions.append(action, null);
								}

								onParamsChanged: {
									if (action.next !== null) {
										// existing action
										if (event.definition !== null) {
											// valid row
											recompile();
										}
										return;
									}

									if (event.definition !== null) {
										var prev;
										if (action.prev === null) {
											// first action
											prev = event
											ast.blocks.push(event);
											ast.links.push(createLink(event, action));
										} else {
											prev = action.prev;
										}
										prev.outbound.destBlock = action;

										ast.blocks.push(action);
										ast.links.push(createLink(action, event));

										recompile();
									}

									action.reserve();
									row.reserve();
								}
							}
						}
					}
					Component {
						id: linkComponent
						Link {
							visible: false
						}
					}
				}
			}
		}
	}
}
