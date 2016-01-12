import QtQuick 2.0

Item {
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
			var starts = [];
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

				if (block.isStarting) {
					starts.push(i);
				}

				subs[i] = {
					"compiled": compiled,
					"parents": [],
					"children": [],
					"thread": -1,
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

			starts.forEach(function(start, thread) {
				var lastIndex = subs.length;
				var lastSub = {
					"compiled": {
						"action": "",
					},
					"parents": [],
					"children": [],
					"thread": thread,
				};
				scene.applyToClique(blocks[start], function(block) {
					var index = indices[block];
					var sub = subs[index];
					sub.thread = thread;
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
			});

			var src = "";
			src += "var states[" + starts.length + "] = [" + starts.map(function() { return -1; }) + "]" + "\n";
			src += "var ages[" + starts.length + "] = [" + starts.map(function() { return -1; }) + "]" + "\n";
			src += "timer.period[0] = 10" + "\n";
			for (var i = subs.length - starts.length; i < subs.length; ++i) {
				src += "callsub block" + i + "\n";
			}
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
					source += "states[" + sub.thread + "] = " + index + "\n";
					source += "ages[" + sub.thread + "] = 0\n";
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
					starts.forEach(function(start, thread) {
						source += "ages[" + thread + "] += 1\n";
					});
				}
				source = events[eventName].reduce(function(source, subIndex) {
					var sub = subs[subIndex];
					var thread = sub.thread;
					source += "if " + sub.parents.reduce(function(expr, arrow) {
						return expr + " or states[" + thread + "] == " + arrow.head;
					}, "0 != 0") + " then" + "\n";
					source += "emit link [states[" + thread + "], " + subIndex + "]\n";
					source += "callsub block" + subIndex + "\n";
					source += "end" + "\n";
					return source;
				}, source);
				return source;
			}, src);
			compiler.source = src;
		}
	}
}
