import QtQuick 2.0

Item {
	id: compiler

	// input
	property var ast

	// output
	property string error
	property var events
	property string script

	// internal
	property var nodes

	onAstChanged: {
		var blocks = ast.blocks;
		for (var i = 0; i < blocks.length; ++i) {
			var block = blocks[i];
			block.paramsChanged.connect(timer.start);
			block.isStartingChanged.connect(timer.start);
		}

		var links = ast.links;
		for (var i = 0; i < links.length; ++i) {
			var link = links[i];
			link.isElseChanged.connect(timer.start);
		}

		timer.start();
	}

	Timer {
		id: timer
		interval: 0
		onTriggered: {
			try {
				var compiled = compile(ast.blocks, ast.links);
				error = "";
				events = compiled.events;
				script = compiled.script;
				nodes = compiled.nodes;
			} catch(err) {
				if (typeof(err) === "string") {
					error = err;
					events = {};
					script = "";
					nodes = [];
				} else {
					throw err;
				}
			}
		}

		function compile(blocks, links) {
			function filledArray(length, value) {
				var array = new Array(length);
				for (var i = 0; i < length; ++i) {
					array[i] = value;
				}
				return array;
			}

			var indices = {};
			var nodes = [];
			var starts = [];
			for (var i = 0; i < blocks.length; ++i) {
				var block = blocks[i];
				block.isError = false;
				indices[block] = i;
				nodes.push({
					blockIndex: i,
					compiled: block.definition.compile(block.params),
					heads: [],
					tails: [],
					thread: -1,
					conditions: [],
					transitions: [],
					events: {},
				});
				if (block.isStarting) {
					starts.push(i);
				}
			}

			for (var i = 0; i < links.length; ++i) {
				var link = links[i];
				var head = indices[link.sourceBlock];
				var tail = indices[link.destBlock];
				var negate = link.isElse;
				nodes[head].tails.push({
					linkIndex: i,
					negate: negate,
					tail: tail,
				});
				nodes[tail].heads.push({
					linkIndex: i,
					negate: negate,
					head: head,
				});
			}

			if (blocks.length === 0) {
				return {
					events: {},
					script: "",
					nodes: [],
				};
			}

			function visitTails(node, visit) {
				node.tails.forEach(function(edge) {
					var index = edge.tail;
					var node = nodes[index];
					visit(node, index, edge);
				});
			}

			// detect infinite loops
			nodes.forEach(function(node, index) {
				if (node.compiled.condition !== undefined && starts.indexOf(index) == -1)
					return;

				var events = [];
				function visitEvents(node, index) {
					if (node.compiled.condition === undefined) {
						return;
					}
					if (events.indexOf(index) !== -1) {
						events.forEach(function(index) {
							var block = blocks[index];
							block.isError = true;
						});
						throw "Infinite loop";
					}
					events.push(index);
					visitTails(node, visitEvents);
					events.pop();
				}
				visitTails(node, visitEvents);

				var states = [];
				function visitStates(node, index) {
					if (node.compiled.condition !== undefined) {
						return;
					}
					if (states.indexOf(index) !== -1) {
						states.forEach(function(index) {
							var block = blocks[index];
							block.isError = true;
						});
						throw "Infinite loop";
					}
					states.push(index);
					visitTails(node, visitStates);
					states.pop();
				}
				visitTails(node, visitStates);
			});

			// detect unreachable blocks
			(function() {
				var visited = [];
				starts.forEach(function(start, thread) {
					function visit(index) {
						if (visited.indexOf(index) !== -1) {
							return;
						}
						visited.push(index);
						nodes[index].tails.forEach(function(edge) {
							visit(edge.tail);
						});
					}
					visit(start);
				});
				var unreachable = 0;
				nodes.forEach(function(node, index) {
					if (visited.indexOf(index) === -1) {
						unreachable += 1;
						var block = blocks[index];
						block.isError = true;
					}
				});
				if (unreachable !== 0) {
					throw "Unreachable blocks";
				}
			})();

			// create start and end states
			starts.forEach(function(index, thread) {
				var node = nodes[index];

				if (node.compiled.condition !== undefined) {
					// start block is an event => add a start state
					var start = nodes.length;
					nodes.push({
						blockIndex: node.blockIndex,
						compiled: {},
						heads: [],
						tails: [{
							linkIndex: -1,
							negate: false,
							tail: index,
						}],
						thread: thread,
						conditions: [],
						transitions: [],
						events: {},
					});
					node.heads.push({
						linkIndex: -1,
						negate: false,
						head: start,
					});
					starts[thread] = start;
				}

				var seen = [];
				function visit(node, index) {
					if (seen.indexOf(index) !== -1) {
						return;
					}
					seen.push(index);
					node.thread = thread;
					if (node.tails.length === 0) {
						if (node.compiled.condition !== undefined) {
							// end block is an event => add an end state
							var end = nodes.length;
							nodes.push({
								blockIndex: node.blockIndex,
								compiled: {},
								heads: [{
									linkIndex: -1,
									negate: false,
									head: index,
								}],
								tails: [],
								thread: thread,
								conditions: [],
								transitions: [],
								events: {},
							});
							node.tails.push({
								linkIndex: -1,
								negate: false,
								tail: end,
							});
						}
					} else {
						visitTails(node, visit);
					}
				}
				visit(node, index);
			});

			var eventStates = {};
			nodes.forEach(function(node, index) {
				if (node.compiled.condition !== undefined)
					return;

				var headIndex = index;
				var head = node;

				var edges = [];
				var indices = [];
				function buildTransition(node, index, edge) {
					edges.push(edge);
					if (node.compiled.condition === undefined) {
						var transitionIndex = head.transitions.length;
						head.transitions.push({
							edges: edges.slice(),
							indices: indices.slice(),
							tail: index,
						});
						for (var i = 0; i < indices.length; ++i) {
							var conditionIndex = indices[i];
							var event = nodes[head.conditions[conditionIndex]].compiled.event;
							var transitions = head.events[event].transitions;
							if (transitions.indexOf(transitionIndex) === -1) {
								transitions.push(transitionIndex);
							}
						}
					} else {
						var conditionIndex = head.conditions.indexOf(index);
						if (conditionIndex === -1) {
							// first use of this condition from this state
							conditionIndex = head.conditions.length;
							head.conditions.push(index);

							var event = node.compiled.event;

							var stateEvents = head.events[event];
							if (stateEvents === undefined) {
								stateEvents = {
									conditions: [],
									transitions: [],
								};
								head.events[event] = stateEvents;
							}
							stateEvents.conditions.push(conditionIndex);

							var states = eventStates[event];
							if (states === undefined) {
								eventStates[event] = [headIndex];
							} else if (states.indexOf(headIndex) === -1) {
								states.push(headIndex);
							}
						}

						indices.push(conditionIndex);
						visitTails(node, buildTransition);
						indices.pop();
					}
					edges.pop();
				}
				visitTails(node, buildTransition);
			});

			var events = {
				"transition": 2,
			};

			var script = "";
			script = Object.keys(eventStates).reduce(function(script, eventName, eventIndex) {
				script += "const event_" + eventName + " = " + eventIndex + "\n";
				return script;
			}, script);
			script += "var thread = -1" + "\n";
			script += "var event = -1" + "\n";
			script += "var states[" + starts.length + "] = [" + starts.join(",") + "]" + "\n";
			script += "var ages[" + starts.length + "] = [" + starts.map(function() { return "0"; }).join(",") + "]" + "\n";
			script += "var conditions = -1" + "\n";
			script += "var transitions[" + starts.length + "] = [" + starts.map(function() { return "0"; }).join(",") + "]" + "\n";
			script += "var transitionsOld = -1" + "\n";
			script += "var transitionsNew = -1" + "\n";
			script += "timer.period[0] = 10" + "\n";
			script = nodes.reduce(function(script, node, index) {
				var global = node.compiled.global;
				if (global !== undefined) {
					script += "\n";
					script += global + "\n";
				}
				return script;
			}, script);
			script = nodes.reduce(function(script, node, index) {
				if (node.compiled.condition !== undefined) {
					return script;
				}

				var thread = node.thread;

				script += "\n";

				script += "sub test" + index + "\n";
				script += "conditions = 0" + "\n";
				script = node.conditions.reduce(function (script, nodeIndex, conditionIndex) {
					var node = nodes[nodeIndex];
					var conditionMask = filledArray(16, "0");
					conditionMask[conditionIndex] = "1";
					script += "if " + node.compiled.condition + " then" + "\n";
					script += "conditions |= 0b" + conditionMask.join("") + "\n";
					script += "end" + "\n";
					return script;
				}, script);

				script += "sub enter" + index + "\n";
				script += "thread = " + thread + "\n";
				script += "states[" + thread + "] = " + index + "\n";
				script += "ages[" + thread + "] = 0" + "\n";
				if (node.compiled.action !== undefined) {
					script += node.compiled.action + "\n";
				}
				script += "callsub test" + index + "\n";

				script += "transitionsNew = 0" + "\n";
				script = node.transitions.reduce(function (script, transition, transitionIndex) {
					if (transition.indices.length === 0) {
						// unconditional transition
						script += "event = -1" + "\n";
						script += "emit transition [" + index + ", " + transitionIndex + "]" + "\n";
						script += "callsub enter" + transition.tail + "\n";
						script += "return" + "\n";
						return script;
					}

					var positiveMask = filledArray(16, "0");
					var negativeMask = filledArray(16, "0");
					transition.indices.forEach(function(conditionIndex, i) {
						if (transition.edges[i + 1].negate) {
							negativeMask[conditionIndex] = "1";
						} else {
							positiveMask[conditionIndex] = "1";
						}
					});
					var positiveTest = "conditions & 0b" + positiveMask.join("") + " == 0b" + positiveMask.join("");
					var negativeTest = "conditions | ~0b" + negativeMask.join("") + " == ~0b" + negativeMask.join("");

					var transitionMask = filledArray(16, "0");
					transitionMask[transitionIndex] = "1";

					script += "if (" + positiveTest + ") and (" + negativeTest + ") then" + "\n";
					script += "transitionsNew |= 0b" + transitionMask.join("") + "\n";
					script += "end" + "\n";
					return script;
				}, script);
				script += "transitions[" + thread + "] = transitionsNew" + "\n";

				return script;
			}, script);
			script = Object.keys(eventStates).reduce(function(script, eventName) {
				script += "\n";
				script += "onevent " + eventName + "\n";
				script += "event = event_" + eventName + "\n";
				if (eventName === "timer0") {
					script += "ages += [" + starts.map(function() { return "1"; }) + "]" + "\n";
				}
				script = eventStates[eventName].reduce(function(script, nodeIndex) {
					var node = nodes[nodeIndex];
					var thread = node.thread;
					var event = node.events[eventName];

					script += "if states[" + thread + "] == " + nodeIndex + " then" + "\n";
					script += "thread = " + thread + "\n";

					script += "callsub test" + nodeIndex + "\n";

					var transitionsMask = filledArray(16, "0");
					event.transitions.forEach(function(transitionIndex) {
						transitionsMask[transitionIndex] = "1";
					});
					script += "transitionsOld = transitions[" + thread + "]" + "\n";
					script += "transitionsNew = transitionsOld & ~0b" + transitionsMask.join("") + "\n";
					script = event.transitions.reduce(function (script, transitionIndex) {
						var transition = node.transitions[transitionIndex];

						var positiveMask = filledArray(16, "0");
						var negativeMask = filledArray(16, "0");
						transition.indices.forEach(function(conditionIndex, i) {
							if (transition.edges[i + 1].negate) {
								negativeMask[conditionIndex] = "1";
							} else {
								positiveMask[conditionIndex] = "1";
							}
						});
						var positiveTest = "conditions & 0b" + positiveMask.join("") + " == 0b" + positiveMask.join("");
						var negativeTest = "conditions | ~0b" + negativeMask.join("") + " == ~0b" + negativeMask.join("");

						var transitionMask = filledArray(16, "0");
						transitionMask[transitionIndex] = "1";

						script += "if (" + positiveTest + ") and (" + negativeTest + ") then" + "\n";
						script += "transitionsNew |= 0b" + transitionMask.join("") + "\n";
						script += "if transitionsOld & 0b" + transitionMask.join("") + " == 0 then" + "\n";
						script += "event = -1" + "\n";
						script += "emit transition [" + nodeIndex + ", " + transitionIndex + "]" + "\n";
						script += "callsub enter" + transition.tail + "\n";
						script += "return" + "\n";
						script += "end" + "\n";
						script += "end" + "\n";
						return script;
					}, script);
					script += "transitions[" + thread + "] = transitionsNew" + "\n";

					script += "end" + "\n";

					return script;
				}, script);
				return script;
			}, script);

			return {
				events: events,
				script: script,
				nodes: nodes,
			};
		}
	}

	function execReset(playing) {
		for (var i = 0; i < blocks.length; ++i) {
			var block = blocks[i];
			if (playing) {
				var isStarting = block.isStarting;
				var isState = nodes[i].compiled.condition === undefined;
				block.isExec = isStarting && isState;
				block.startIndicator.isExec = isStarting && !isState;
			} else {
				block.isExec = false;
				block.startIndicator.isExec = false;
			}
		}
	}

	function execTransition(nodeIndex, transitionIndex) {
		var node = nodes[nodeIndex];
		var blockIndex = node.blockIndex;
		var block = blocks[blockIndex];

		if (nodeIndex === blockIndex) {
			// leaving a state block
			block.isExec = false;
		} else {
			// leaving a start indicator
			block.startIndicator.isExec = false;
		}

		var edges = node.transitions[transitionIndex].edges;
		for (var i = 0; i < edges.length; ++i) {
			var edge = edges[i];
			var linkIndex = edge.linkIndex;
			if (linkIndex !== -1) {
				var link = links[linkIndex];
				link.exec();
			}
			nodeIndex = edge.tail;

			node = nodes[nodeIndex];
			blockIndex = node.blockIndex;
			block = blocks[blockIndex];

			if (i !== edges.length - 1) {
				block.exec();
			}
		}

		if (nodeIndex === blockIndex) {
			// entering a state block
			block.isExec = true;
		}
	}

}
