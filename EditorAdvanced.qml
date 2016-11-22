import QtQuick 2.7

// container for main view
Item {
	id: scene

	property int highestZ: 2

	property real vx: 0 // in px per second
	property real vy: 0 // in px per second

	property var ast: ({
		blocks: blocks,
		links: links,
	})

	readonly property rect viewRect: blockContainer.childrenRect

	readonly property alias blocks: blockContainer.children
	readonly property alias links: linkContainer.children

	readonly property real repulsionMaxDist: 330

	// return a JSON representation of the content of the editor
	function serialize() {
		return serializeAdvanced();
	}

	// reset content from a JSON representation
	function deserialize(data) {

		// first restore blocks
		blocks = [];
		for (var i = 0; i < data.blocks.length; ++i) {
			var b = data.blocks[i];
			blockComponent.createObject(blockContainer, {
				x: b.x,
				y: b.y,
				definition: definitions[b.definition],
				params: b.params,
				isStarting: b.isStarting
			});
		}

		// add then links
		links = [];
		for (var i = 0; i < data.links.length; ++i) {
			var l = data.links[i];
			blockLinkComponent.createObject(linkContainer, {
				sourceBlock: blocks[l.source],
				destBlock: blocks[l.dest],
				isElse: l.isElse
			});
		}
	}

	function clear() {
		links = [];
		blocks = [];
	}

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
	}

	// container for all blocks
	Item {
		id: blockContainer
		anchors.fill: parent

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
				for (i = 0; i < blocks.length; ++i) {
					for (j = 0; j < i; ++j) {
						var dx = blocks[i].x - blocks[j].x;
						var dy = blocks[i].y - blocks[j].y;
						var dist = Math.sqrt(dx*dx + dy*dy);
						if (dist < repulsionMaxDist) {
							var normDist = dist;
							var factor = 100 / (normDist+1);
							blocks[i].x += sign(dx) * factor;
							blocks[j].x -= sign(dx) * factor;
							blocks[i].y += sign(dy) * factor;
							blocks[j].y -= sign(dy) * factor;
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

	Component {
		id: blockComponent
		Block {
		}
	}

	Binding {
		target: mainDropArea
		property: "enabled"
		value: true
	}
	Connections {
		target: mainDropArea
		onDropped: {
			if (drop.source === blockDragPreview) {
				var pos = mainDropArea.mapToItem(scene, drop.x, drop.y);
				createBlock(pos.x, pos.y, blockDragPreview.definition);
			} else {
				drop.accepted = false;
			}
		}
	}

	function handleBlockDrop(source, drop) {
		if (drop.source === blockDragPreview) {
			drop.accepted = true;
			var pos = source.mapToItem(scene, drop.x, drop.y);
			var dest = createBlock(pos.x, pos.y, blockDragPreview.definition);
			dest.isStarting = false;
			var linkProperties = {
				sourceBlock: source,
				destBlock: dest,
			};
			blockLinkComponent.createObject(linkContainer, linkProperties);
		} else {
			drop.accepted = false;
		}
	}
}
