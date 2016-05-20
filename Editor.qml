import QtQuick 2.5
import QtQuick.Window 2.2
import QtGraphicalEffects 1.0
import Qt.labs.controls 1.0
import "blocks"

Item {
	id: vplEditor

	// whether VPL editor is minimized on top of background image
	property bool minimized: false

	property Compiler compiler: Compiler {}

	readonly property alias blocks: blockContainer.children
	readonly property alias links: linkContainer.children

	readonly property real repulsionMaxDist: 330

	function execLink(sourceBlockIndex, targetBlockIndex) {
		if (sourceBlockIndex >= blockContainer.children.length)
			return;
		if (targetBlockIndex >= blockContainer.children.length)
			return;
		var sourceBlock = blockContainer.children[sourceBlockIndex];
		var targetBlock = blockContainer.children[targetBlockIndex];
		for (var i = 0; i < linkContainer.children.length; ++i) {
			var link = linkContainer.children[i];
			if (link.sourceBlock === sourceBlock && link.destBlock === targetBlock)
				link.exec();
		}
	}

	function execBlock(blockIndex, isTrue) {
		if (blockIndex < blocks.length) {
			blocks[blockIndex].exec(isTrue);
		} else {
			var threadIndex = blockIndex - blocks.length;
			for (var i = 0; i < blocks.length; ++i) {
				var block = blocks[i];
				if (block.isStarting) {
					if (threadIndex === 0) {
						block.startIndicator.exec();
						break;
					}
					threadIndex -= 1;
				}
			}
		}
	}

	BlocksPane {
		id: eventPane

		blocks: editor.eventBlocks
		backImage: "images/eventCenter.svg"
		x: vplEditor.minimized ? -width : 0

		Behavior on x { PropertyAnimation {} }
	}

	BlocksPane {
		id: actionPane

		blocks: editor.actionBlocks
		backImage: "images/actionCenter.svg"
		x: vplEditor.minimized ? parent.width : parent.width - width

		Behavior on x { PropertyAnimation {} }
	}

	Rectangle {
		id: mainContainer

		property real foregroundWidth: parent.width - eventPane.width - actionPane.width

		anchors.right: parent.right
		anchors.bottom: parent.bottom
		anchors.rightMargin: vplEditor.minimized ? 0 : actionPane.width

		width: foregroundWidth
		height: parent.height
		//opacity: vplEditor.minimized ? 0.5 : 1.0
		color: vplEditor.minimized ? "#80200032" : "#ff44285a"
		scale: vplEditor.minimized ? 0.5 : 1.0
		transformOrigin: Item.BottomRight

		Behavior on opacity { PropertyAnimation {} }
		Behavior on color { PropertyAnimation {} }
		Behavior on scale { PropertyAnimation {} }
		Behavior on anchors.rightMargin { PropertyAnimation {} }

		Image {
			anchors.fill: parent
			source: "images/grid.png"
			fillMode: Image.Tile
			opacity: vplEditor.minimized ? 0 : 1

			Behavior on opacity { PropertyAnimation {} }
		}

		// container for main view
		PinchArea {
			id: pinchArea

			anchors.fill: parent
			clip: true

			property double prevTime: 0

			onPinchStarted: {
				prevTime = new Date().valueOf();
			}

			onPinchUpdated: {
				var deltaScale = pinch.scale - pinch.previousScale

				// adjust content pos due to scale
				if (scene.scale + deltaScale > 1e-1) {
					scene.x += (scene.x - pinch.center.x) * deltaScale / scene.scale;
					scene.y += (scene.y - pinch.center.y) * deltaScale / scene.scale;
					scene.scale += deltaScale;
				}

				// adjust content pos due to drag
				var now = new Date().valueOf();
				var dt = now - prevTime;
				var dx = pinch.center.x - pinch.previousCenter.x;
				var dy = pinch.center.y - pinch.previousCenter.y;
				scene.x += dx;
				scene.y += dy;
				//scene.vx = scene.vx * 0.6 + dx * 0.4 * dt;
				//scene.vy = scene.vy * 0.6 + dy * 0.4 * dt;
				prevTime = now;
			}

			onPinchFinished: {
				//accelerationTimer.running = true;
			}

			MouseArea {
				anchors.fill: parent
				drag.target: scene
				scrollGestureEnabled: false

				onWheel: {
					var deltaScale = scene.scale * wheel.angleDelta.y / 1200.;

					// adjust content pos due to scale
					if (scene.scale + deltaScale > 1e-1) {
						scene.x += (scene.x - mainContainer.width/2) * deltaScale / scene.scale;
						scene.y += (scene.y - mainContainer.height/2) * deltaScale / scene.scale;
						scene.scale += deltaScale;
					}
				}
			}

			Item {
				id: scene

				property int highestZ: 2

				property real vx: 0 // in px per second
				property real vy: 0 // in px per second

				scale: 0.5

				// we use a timer to have some smooth effect
				// TODO: fixme
				Timer {
					id: accelerationTimer
					interval: 17
					repeat: true
					onTriggered: {
						x += (vx * interval) * 0.001;
						y += (vy * interval) * 0.001;
						vx *= 0.85;
						vy *= 0.85;
						if (Math.abs(vx) < 1 && Math.abs(vy) < 1)
						{
							running = false;
							vx = 0;
							vy = 0;
						}
						console.log(vx);
						console.log(vy);
					}
				}

				// methods for querying and modifying block and link graph

				// apply func to every block in the clique starting from block, considering links as undirected
				function applyToClique(block, func, excludedLink) {
					// build block to outgoing links map
					var blockLinks = {};
					for (var i = 0; i < links.length; ++i) {
						var link = links[i];
						if (link === excludedLink)
							continue;
						if (!(link.sourceBlock in blockLinks)) {
							blockLinks[link.sourceBlock] = {};
						}
						// note: we use the same object for key and value in order to access it later
						blockLinks[link.sourceBlock][link.destBlock] = link.destBlock;
						if (!(link.destBlock in blockLinks)) {
							blockLinks[link.destBlock] = {};
						}
						// note: we use the same object for key and value in order to access it later
						blockLinks[link.destBlock][link.sourceBlock] = link.sourceBlock;
					}
					// set of seens blocks
					var seenBlocks = {};
					// recursive function to process each block
					function processBlock(block) {
						if (block in seenBlocks)
							return;
						func(block);
						seenBlocks[block] = true;
						if (!(block in blockLinks))
							return;
						var nextBlocks = blockLinks[block];
						Object.keys(nextBlocks).forEach(function(nextBlockString) {
							var nextBlock = nextBlocks[nextBlockString];
							processBlock(nextBlock);
						});
					}
					// run from the passed block
					processBlock(block);
				}
				function areBlocksInSameClique(block0, block1, excludedLink) {
					var areInSameClique = false;
					applyToClique(block0, function (block) { if (block === block1) { areInSameClique = true; } }, excludedLink);
					return areInSameClique;
				}

				// methods for updating link indicators

				// set starting indicator on either the sourceBlock or the destBlock clique
				function removeLink(link) {
					// if after link is removed the blocks are in the same clique, do not do anything
					if (!areBlocksInSameClique(link.sourceBlock, link.destBlock, link)) {
						// blocks are in different cliques
						var leftHasStart = false;
						applyToClique(link.sourceBlock, function (block) { if (block.isStarting) leftHasStart = true; }, link);
						if (leftHasStart) {
							link.destBlock.isStarting = true;
						} else {
							link.sourceBlock.isStarting = true;
						}
					}
					link.destroy();
				}
				// if the two blocks will form a united clique, clear the start indicator of old destination clique
				function joinClique(sourceBlock, destBlock, excludedLink) {
					var touching = false;
					scene.applyToClique(sourceBlock, function (block) { touching = touching || (block === destBlock); }, excludedLink);
					if (!touching) {
						scene.applyToClique(destBlock, function (block) { block.isStarting = false; }, excludedLink);
					}
				}

				// create a new block at given coordinates
				function createBlock(x, y, definition) {
					var block = blockComponent.createObject(blockContainer, {
						x: x - 128 + Math.random(),
						y: y - 128 + Math.random(),
						definition: definition,
						params: definition.defaultParams
					});
					return block;
				}

				// delete a block and all its links from the scene
				function deleteBlock(block) {
					// yes, collect all links and arrows from/to this block
					var toDelete = []
					for (var i = 0; i < linkContainer.children.length; ++i) {
						var child = linkContainer.children[i];
						// if so, collect for removal
						if (child.sourceBlock === block || child.destBlock === block)
							toDelete.push(child);
					}
					// remove collected links and arrows
					for (i = 0; i < toDelete.length; ++i)
						toDelete[i].destroy();
					// remove this block from the scene
					block.destroy();
				}

				// container for all links
				Item {
					id: linkContainer
					onChildrenChanged: compiler.compile()
				}

				// container for all blocks
				Item {
					id: blockContainer
					onChildrenChanged: compiler.compile()

					// timer to desinterlace objects
					Timer {
						interval: 17
						repeat: true
						running: true

						function sign(v) {
							if (v > 0)
								return 1;
							else if (v < 0)
								return -1;
							else
								return 0;
						}

						onTriggered: {
							var i, j;
							// move all blocks too close
							for (i = 0; i < blockContainer.children.length; ++i) {
								for (j = 0; j < i; ++j) {
									var dx = blockContainer.children[i].x - blockContainer.children[j].x;
									var dy = blockContainer.children[i].y - blockContainer.children[j].y;
									var dist = Math.sqrt(dx*dx + dy*dy);
									if (dist < repulsionMaxDist) {
										var normDist = dist;
										var factor = 100 / (normDist+1);
										blockContainer.children[i].x += sign(dx) * factor;
										blockContainer.children[j].x -= sign(dx) * factor;
										blockContainer.children[i].y += sign(dy) * factor;
										blockContainer.children[j].y -= sign(dy) * factor;
									}
								}
							}
						}
					}
				}

				Component {
					id: blockLinkComponent
					Link { }
				}
			}
		}

		// preview when adding block
		Item {
			id: blockDragPreview
			property BlockDefinition definition: null
			property alias params: loader.defaultParams
			property string backgroundImage
			width: 256
			height: 256
			visible: definition !== null
			scale: scene.scale

			HDPIImage {
				id: centerImageId
				source: blockDragPreview.backgroundImage
				anchors.centerIn: parent
				scale: 0.72
				width: 256 // working around Qt bug with SVG and HiDPI
				height: 256 // working around Qt bug with SVG and HiDPI
			}

			Loader {
				id: loader
				property var defaultParams
				anchors.centerIn: parent
				sourceComponent: blockDragPreview.definition ? blockDragPreview.definition.miniature : null
				scale: 0.72
			}
		}

		Component {
			id: blockComponent
			Block {
			}
		}

		// center view
		HDPIImage {
			source: "images/ic_gps_fixed_white_24px.svg"
			width: 24 // working around Qt bug with SVG and HiDPI
			height: 24 // working around Qt bug with SVG and HiDPI
			anchors.right: parent.right
			anchors.rightMargin: 12
			anchors.bottom: parent.bottom
			anchors.bottomMargin: 12+24+24
			visible: !minimized

			MouseArea {
				anchors.fill: parent
				onClicked: {
					if (blockContainer.children.length === 0)
						return;
					scene.scale = Math.min(mainContainer.width * 0.7 / blockContainer.childrenRect.width, mainContainer.height * 0.7 / blockContainer.childrenRect.height);
					scene.x = mainContainer.width/2 - (blockContainer.childrenRect.x + blockContainer.childrenRect.width/2) * scene.scale;
					scene.y = mainContainer.height/2 - (blockContainer.childrenRect.y + blockContainer.childrenRect.height/2) * scene.scale;
				}
			}
		}

		// clear content
		HDPIImage {
			source: "images/ic_clear_white_24px.svg"
			width: 24 // working around Qt bug with SVG and HiDPI
			height: 24 // working around Qt bug with SVG and HiDPI
			anchors.right: parent.right
			anchors.rightMargin: 12
			anchors.bottom: parent.bottom
			anchors.bottomMargin: 12
			visible: !minimized

			MouseArea {
				anchors.fill: parent
				onClicked: {
					linkContainer.children = [];
					blockContainer.children = [];
					scene.scale = 0.5;
				}
			}
		}

		// block editor
		BlockEditor {
			id: editor
			z: 1
		}
	}
}
