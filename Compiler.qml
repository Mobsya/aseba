import QtQuick 2.0

Item {
	id: compiler
	property var nodes
	property string source
	property string error

	function compile() {
		timer.start();
	}

	Timer {
		id: timer
		interval: 0
		onTriggered: {
			try {
				var compiled = compile();
				compiler.nodes = compiled.nodes;
				compiler.source = compiled.source;
				compiler.error = "";
			} catch(error) {
				if (typeof(error) === "string") {
					compiler.nodes = [];
					compiler.source = "";
					compiler.error = error;
				} else {
					throw error;
				}
			}
		}

		function compile() {
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
					nodes: [],
					source: "",
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
				if (node.compiled.condition !== undefined)
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

			var src = "";
			src += "var thread = -1" + "\n";
			src += "var states[" + starts.length + "] = [" + starts.map(function() { return "-1"; }).join(",") + "]" + "\n";
			src += "var ages[" + starts.length + "] = [" + starts.map(function() { return "-1"; }).join(",") + "]" + "\n";
			src += "var conditions[" + starts.length + "] = [" + starts.map(function() { return "-1"; }).join(",") + "]" + "\n";
			src += "var conditionsOld = -1" + "\n";
			src += "var conditionsNew = -1" + "\n";
			src += "var transitions[" + starts.length + "] = [" + starts.map(function() { return "-1"; }).join(",") + "]" + "\n";
			src += "var transitionsOld = -1" + "\n";
			src += "var transitionsNew = -1" + "\n";
			src += "timer.period[0] = 10" + "\n";
			for (var i = 0; i < starts.length; ++i) {
				var start = starts[i];
				src += "callsub state" + start + "\n";
			}
			src = nodes.reduce(function(source, node, index) {
				var global = node.compiled.global;
				if (global !== undefined) {
					source += "\n";
					source += global + "\n";
				}
				return source;
			}, src);
			src = nodes.reduce(function(source, node, index) {
				if (node.compiled.condition !== undefined) {
					return source;
				}

				var thread = node.thread;

				source += "\n";
				source += "sub state" + index + "\n";
				source += "thread = " + thread + "\n";
				source += "states[" + thread + "] = " + index + "\n";
				source += "ages[" + thread + "] = 0" + "\n";
				if (node.compiled.action !== undefined) {
					source += node.compiled.action + "\n";
				}

				source += "conditionsNew = 0" + "\n";
				source = node.conditions.reduce(function (source, nodeIndex, conditionIndex) {
					var condition = nodes[nodeIndex].compiled.condition;
					var conditionMask = filledArray(16, "0");
					conditionMask[conditionIndex] = "1";
					source += "if " + condition + " then" + "\n";
					source += "conditionsNew |= 0b" + conditionMask.join("") + "\n";
					source += "end" + "\n";
					return source;
				}, source);
				source += "conditions[" + thread + "] = conditionsNew" + "\n";

				source += "transitionsNew = 0" + "\n";
				source = node.transitions.reduce(function (source, transition, transitionIndex) {
					if (transition.indices.length === 0) {
						// unconditional transition
						source += "emit transition [" + index + ", " + transitionIndex + "]" + "\n";
						source += "callsub state" + transition.tail + "\n";
						return source;
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
					var positiveTest = "conditionsNew & 0b" + positiveMask.join("") + " == 0b" + positiveMask.join("");
					var negativeTest = "conditionsNew | ~0b" + negativeMask.join("") + " == ~0b" + negativeMask.join("");

					var transitionMask = filledArray(16, "0");
					transitionMask[transitionIndex] = "1";

					source += "if (" + positiveTest + ") and (" + negativeTest + ") then" + "\n";
					source += "transitionsNew |= 0b" + transitionMask.join("") + "\n";
					source += "end" + "\n";
					return source;
				}, source);
				source += "transitions[" + thread + "] = transitionsNew" + "\n";

				return source;
			}, src);
			src = Object.keys(eventStates).reduce(function(source, eventName) {
				source += "\n";
				source += "onevent " + eventName + "\n";
				if (eventName === "timer0") {
					source += "ages += [" + starts.map(function() { return "1"; }) + "]" + "\n";
				}
				source = eventStates[eventName].reduce(function(source, nodeIndex) {
					var node = nodes[nodeIndex];
					var thread = node.thread;
					var event = node.events[eventName];

					source += "if states[" + thread + "] == " + nodeIndex + " then" + "\n";
					source += "thread = " + thread + "\n";

					var conditionsMask = filledArray(16, "0");
					event.conditions.forEach(function(conditionIndex) {
						conditionsMask[conditionIndex] = "1";
					});
					source += "conditionsOld = conditions[" + thread + "]" + "\n";
					source += "conditionsNew = conditionsOld & ~0b" + conditionsMask.join("") + "\n";
					source = event.conditions.reduce(function(source, conditionIndex) {
						var condition = nodes[node.conditions[conditionIndex]].compiled.condition;
						var conditionMask = filledArray(16, "0");
						conditionMask[conditionIndex] = "1";
						source += "if " + condition + " then" + "\n";
						source += "conditionsNew |= 0b" + conditionMask.join("") + "\n";
						source += "end" + "\n";
						return source;
					}, source);
					source += "conditions[" + thread + "] = conditionsNew" + "\n";

					var transitionsMask = filledArray(16, "0");
					event.transitions.forEach(function(transitionIndex) {
						transitionsMask[transitionIndex] = "1";
					});
					source += "transitionsOld = transitions[" + thread + "]" + "\n";
					source += "transitionsNew = transitionsOld & ~0b" + transitionsMask.join("") + "\n";
					source = event.transitions.reduce(function (source, transitionIndex) {
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
						var positiveTest = "conditionsNew & 0b" + positiveMask.join("") + " == 0b" + positiveMask.join("");
						var negativeTest = "conditionsNew | ~0b" + negativeMask.join("") + " == ~0b" + negativeMask.join("");

						var transitionMask = filledArray(16, "0");
						transitionMask[transitionIndex] = "1";

						source += "if (" + positiveTest + ") and (" + negativeTest + ") then" + "\n";
						source += "transitionsNew |= 0b" + transitionMask.join("") + "\n";
						source += "if transitionsOld & 0b" + transitionMask.join("") + " == 0 then" + "\n";
						source += "emit transition [" + nodeIndex + ", " + transitionIndex + "]" + "\n";
						source += "callsub state" + transition.tail + "\n";
						source += "return" + "\n";
						source += "end" + "\n";
						source += "end" + "\n";
						return source;
					}, source);
					source += "transitions[" + thread + "] = transitionsNew" + "\n";

					source += "end" + "\n";

					return source;
				}, source);
				return source;
			}, src);

			return {
				nodes: nodes,
				source: src,
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
