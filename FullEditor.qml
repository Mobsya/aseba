import QtQuick 2.5
import QtQuick.Window 2.2
import QtGraphicalEffects 1.0

Rectangle {
	id: mainContainer

	readonly property alias blocks: blockContainer.children
	readonly property alias links: linkContainer.children

	readonly property real repulsionMaxDist: 330

	property Item compiler: Item {
		property string source
		function compile() {
			timer.start();
		}

		Timer {
			id: timer
			interval: 0
			onTriggered: {
				var indices = {};
				var events = {};
				var subs = [];
				for (var i = 0; i < blocks.length; ++i) {
					var block = blocks[i];
					indices[block] = i;
					var compiled = block.definition.compile(block.params);

					var eventName = compiled.event;
					if (eventName !== undefined) {
						var eventSubs = events[eventName];
						if (eventSubs === undefined) {
							events[eventName] = [i];
						} else {
							eventSubs.push(i);
						}
					}

					subs[i] = {
						"compiled": compiled,
						"parents": [],
						"children": [],
					};
				}
				for (var i = 0; i < links.length; ++i) {
					var link = links[i];
					var sourceIndex = indices[link.sourceBlock];
					var destIndex = indices[link.destBlock];
					var arrow = {
						"head": sourceIndex,
						"tail": destIndex,
						"isElse": link.isElse,
					};
					subs[sourceIndex].children.push(arrow);
					subs[destIndex].parents.push(arrow);
				}

				var lastIndex = subs.length;
				var lastSub = {
					"compiled": {
						"action": "",
					},
					"parents": [],
					"children": [],
				};
				subs.forEach(function(sub, index) {
					if (sub.parents.length === 0) {
						var arrow = {
							"head": lastIndex,
							"tail": index,
							"isElse": false,
						};
						sub.parents.push(arrow);
						lastSub.children.push(arrow);
					}
					if (sub.children.length === 0) {
						var arrow2 = {
							"head": index,
							"tail": lastIndex,
							"isElse": false,
						};
						sub.children.push(arrow2);
						lastSub.parents.push(arrow2);
					}
				});
				subs[lastIndex] = lastSub;

				var src = "";
				src += "var state = -1" + "\n";
				src += "var age = -1" + "\n";
				src += "timer.period[0] = 10" + "\n";
				src += "callsub block" + lastIndex + "\n";
				src = subs.reduce(function(source, sub, index) {
					var global = sub.compiled.global;
					if (global !== undefined) {
						source += "\n";
						source += global + "\n";
					}
					return source;
				}, src);
				src = subs.reduce(function(source, sub, index) {
					var condition = sub.compiled.condition;
					var action = sub.compiled.action;

					source += "\n";
					source += "sub block" + index + "\n";

					if (condition !== undefined) {
						source += "if " + condition + " then" + "\n";
					}

					source += "emit block [" + index + ", 1]\n";

					if (action !== undefined) {
						source += "state = " + index + "\n";
						source += "age = 0\n";
						source += action + "\n";
					}

					sub.children.forEach(function(arrow) {
						if (arrow.isElse) {
							return;
						}
						var child = subs[arrow.tail];
						if (action === undefined || child.compiled.event === undefined) {
							source += "emit link [" + index + ", " + arrow.tail + "]\n";
							source += "callsub block" + arrow.tail + "\n";
						}
					});

					if (condition !== undefined) {
						source += "else" + "\n";
						source += "emit block [" + index + ", 0]\n";
						sub.children.forEach(function(arrow) {
							if (!arrow.isElse) {
								return;
							}
							var child = subs[arrow.tail];
							if (action === undefined || child.compiled.event === undefined) {
								source += "emit link [" + index + ", " + arrow.tail + "]\n";
								source += "callsub block" + arrow.tail + "\n";
							}
						});
						source += "end" + "\n";
					}

					return source;
				}, src);
				src = Object.keys(events).reduce(function(source, eventName) {
					source += "\n";
					source += "onevent " + eventName + "\n";
					if (eventName === "timer0") {
						source += "age += 1\n";
					}
					source = events[eventName].reduce(function(source, subIndex) {
						var sub = subs[subIndex];
						source += "if " + sub.parents.reduce(function(expr, arrow) {
							return expr + " or state == " + arrow.head;
						}, "0 != 0") + " then" + "\n";
						source += "emit link [state, " + subIndex + "]\n";
						source += "callsub block" + subIndex + "\n";
						source += "end" + "\n";
						return source;
					}, source);
					return source;
				}, src);
				console.warn(src);
				compiler.source = src;
			}
		}
	}

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
		if (blockIndex < blockContainer.children.length) {
			blockContainer.children[blockIndex].exec(isTrue);
		}
	}

	RadialGradient {
			anchors.fill: parent
			gradient: Gradient {
				GradientStop { position: 0.0; color: "white" }
				GradientStop { position: 0.5; color: "#eaeced" }
				//GradientStop { position: 0.0; color: "#1e2551" }
				//GradientStop { position: 0.5; color: "#121729" }
			}
		}

	// container for main view
	PinchArea {
		id: pinchArea

		anchors.fill: parent

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

			property real vx: 0 // in px per millisecond
			property real vy: 0 // in px per millisecond

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
			function applyToClique(block, func, excludedLink) {
				// build block to outgoing links map
				var blockLinks = {};
				for (var i = 0; i < links.length; ++i) {
					var link = links[i];
					if (link == excludedLink)
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
				//console.log("areBlocksInSameClique()");
				// FIXME: why do we need double equal here?
				//applyToClique(block0, function (block) { console.log(block + " " + block1); if (block == block1) { areInSameClique = true; console.log("true"); } }, excludedLink);
				//console.log("areBlocksInSameClique " + block0 + " " + block1 + " : " + areInSameClique);
				applyToClique(block0, function (block) { if (block == block1) { areInSameClique = true; } }, excludedLink);
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
				scene.applyToClique(sourceBlock, function (block) { touching = touching || (block == destBlock); }, excludedLink);
				scene.applyToClique(destBlock, function (block) { touching = touching || (block == sourceBlock); }, excludedLink);
				if (!touching) {
					scene.applyToClique(destBlock, function (block) { block.isStarting = false; }, excludedLink);
				}
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

	// add block
	Image {
		id: addBlock
		source: "images/addButton.svg"

		width: 128
		height: 128
		anchors.left: parent.left
		anchors.leftMargin: 20
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 20

		Rectangle {
			id: dragTarget

			x: -64
			y: -64
			width: 256
			height: 256
			radius: 128
			color: "#70000000"

			visible: addBlockMouseArea.drag.active
		}

		MouseArea {
			id: addBlockMouseArea
			anchors.fill: parent
			drag.target: dragTarget
			property Item highlightedBlock: null

			onClicked: {
				if (editor.visible)
					return;
				var pos = mainContainer.mapToItem(blockContainer, mainContainer.width/2, mainContainer.height/2);
				createBlock(pos.x, pos.y);
			}

			onPressed: {
				if (editor.visible) {
					mouse.accepted  = false;
				} else {
					dragTarget.scale = scene.scale;
				}
			}

			onPositionChanged: {
				// find nearest block
				var scenePos = dragTarget.mapToItem(blockContainer, 128, 128);
				var minDist = Number.MAX_VALUE;
				var minIndex = 0;
				for (var i = 0; i < blockContainer.children.length; ++i) {
					var child = blockContainer.children[i];
					var destPos = child.mapToItem(blockContainer, 128, 128);
					var dx = scenePos.x - destPos.x;
					var dy = scenePos.y - destPos.y;
					var dist = Math.sqrt(dx*dx + dy*dy);
					if (dist < minDist) {
						minDist = dist;
						minIndex = i;
					}
				}

				// if blocks are close
				if (minDist < repulsionMaxDist) {
					var destBlock = blockContainer.children[minIndex];
					// highlight destblock
					if (highlightedBlock && highlightedBlock !== destBlock) {
						highlightedBlock.highlight = false;
					}
					destBlock.highlight = true;
					highlightedBlock = destBlock;
				} else if (highlightedBlock) {
					highlightedBlock.highlight = false;
					highlightedBlock = null;
				}
			}

			onReleased: {
				if (!drag.active)
					return;

				// create block
				var pos = mapToItem(blockContainer, mouse.x, mouse.y);
				var newBlock = createBlock(pos.x, pos.y);

				// create link
				if (highlightedBlock) {
					newBlock.isStarting = false;
					var link = blockLinkComponent.createObject(linkContainer, {
						sourceBlock: highlightedBlock,
						destBlock: newBlock
					});
					// reset highlight
					highlightedBlock.highlight = false;
					highlightedBlock = null;
				}

				// reset indicator
				dragTarget.x = -64;
				dragTarget.y = -64;
			}

			function createBlock(x, y) {
				var block = blockComponent.createObject(blockContainer, {
					x: x - 128 + Math.random(),
					y: y - 128 + Math.random(),
					definition: editor.definition,
					params: editor.params
				});
				editor.block = block;
				return block;
			}
		}
	}

	Component {
		id: blockComponent
		Block {
		}
	}

	// delete block
	Rectangle {
		id: delBlock

		anchors.right: parent.right
		anchors.bottom: parent.bottom

		width: 96+40
		height: 96+40

		color: "transparent"

		Image {
			id: delBlockImage

			source: "images/trashDefault.svg"

			state: parent.state

			width: 96
			height: 96

			anchors.right: parent.right
			anchors.rightMargin: 20
			anchors.bottom: parent.bottom
			anchors.bottomMargin: 20+16
		}

		state: "NORMAL"

		states: [
			State {
				name: "HIGHLIGHTED"
				PropertyChanges { target: delBlockImage; source: "images/trashOpen.svg"; }
			}
		]
	}

	// center view
	Image {
		id: backgroundImage
		source: "images/centerContent.svg"

		width: 80
		height: 80
		anchors.right: parent.right
		anchors.rightMargin: 20+16
		anchors.top: parent.top
		anchors.topMargin: 20+16

		MouseArea {
			anchors.fill: parent
			onClicked: {
				if (blockContainer.children.size === 0)
					return;
				scene.scale = Math.min(mainContainer.width * 0.8 / blockContainer.childrenRect.width, mainContainer.height * 0.8 / blockContainer.childrenRect.height);
				scene.x = mainContainer.width/2 - (blockContainer.childrenRect.x + blockContainer.childrenRect.width/2) * scene.scale;
				scene.y = mainContainer.height/2 - (blockContainer.childrenRect.y + blockContainer.childrenRect.height/2) * scene.scale;
			}
		}
	}

	// block editor
	BlockEditor {
		id: editor
		z: 1
	}
}

