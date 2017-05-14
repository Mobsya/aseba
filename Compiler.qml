import QtQuick 2.0

Item {
	id: compiler

	/*
	  input startStates: State[]
	  State: {
		transitions: Transition[]
		active: boolean
	  }
	  Transition: {
		events: Event[]
		actions: Action[]
		next: State
		trigger(): void
	  }
	  Event: {
		compile(): EventCode
	  }
	  EventCode: {
		events: string[]
		condition: string
	  }
	  Action: {
		compile(): string
	  }
	  */
	property var ast

	// output
	property var output:
		({
			 error: "",
			 events: {},
			 script: "",
		 })

	// internal
	property var internal

	onAstChanged: {
		timer.start();
	}

	Timer {
		id: timer
		interval: 0
		onTriggered: {
			try {
				var result = compile(ast);
				output = result.output;
				internal = result.internal;
			} catch(error) {
				if (typeof(error) === "string") {
					output = {
						error: error,
						events: {},
						script: "",
					};
					internal = {
						states: [],
						transitions: [],
					};
				} else {
					throw error;
				}
			}
		}

		function compile(startStates) {
			function filledArray(length, value) {
				var array = new Array(length);
				for (var i = 0; i < length; ++i) {
					array[i] = value;
				}
				return array;
			}

			var version = {}; // a unique token for each compilation pass
			var transitionDatas = [];
			var stateDatas = [];
			var threadDatas = [];
			var eventThreads = {};

			function visitTransition(thread, transition) {
				var transitionData = transition.compilationData;
				if (transitionData === undefined || transitionData.version !== version) {
					var index = transitionDatas.length;
					transitionData = {
						version: version,
						index: index,
						transition: transition,
						events: [],
						condition: "(0 == 0)",
						actions: "",
						next: {},
					};
					transitionDatas.push(transitionData);
					transition.compilationData = transitionData;

					transition.events.forEach(function (event) {
						var compiled = event.compile();
						if (transitionData.events.indexOf(compiled.event) === -1) {
							transitionData.events.push(compiled.event);
						}
						transitionData.condition += " and (" + compiled.condition + ")";
					});

					transition.actions.forEach(function (action) {
						var compiled = action.compile();
						transitionData.actions += compiled.action + "\n";
					});

					transitionData.next = visitState(thread, transition.next);
				}
				return transitionData;
			}

			function visitState(thread, state) {
				var stateData = state.compilationData;
				if (stateData === undefined || stateData.version !== version) {
					var index = stateDatas.length;
					stateData = {
						version: version,
						index: index,
						state: state,
						thread: thread,
						events: {},
						transitions: [],
					};
					stateDatas.push(stateData);
					state.compilationData = stateData;

					var transitions = state.transitions;
					for (var i = 0; i < transitions.length; ++i) {
						var transition = transitions[i];
						var transitionData = visitTransition(thread, transition);
						stateData.transitions.push(transitionData);

						transitionData.events.forEach(function(event) {
							var events = stateData.events[event];
							if (events === undefined) {
								events = [];
								stateData.events[event] = events;
							}
							events.push(transitionData);
						});
					}

					Object.keys(stateData.events).forEach(function(event) {
						var events = thread.events[event];
						if (events === undefined) {
							events = [];
							thread.events[event] = events;
						}
						events.push(stateData);
					});
				}
				return stateData;
			}

			function visitThread(startState) {
				var index = threadDatas.length;
				var threadData = {
					version: version,
					index: index,
					startState: {},
					events: {},
				};
				threadDatas.push(threadData);

				threadData.startState = visitState(threadData, startState);

				Object.keys(threadData.events).forEach(function(event) {
					var events = eventThreads[event];
					if (events === undefined) {
						events = [];
						eventThreads[event] = events;
					}
					events.push(threadData);
				});
				return threadData;
			}

			for (var i = 0; i < startStates.length; ++i) {
				var startState = startStates[i];
				visitThread(startState);
			}

			var events = {
				"transition": 1,
			};

			var script = "";
			script = Object.keys(eventThreads).reduce(function(script, eventName, eventIndex) {
				script += "const event_" + eventName + " = " + eventIndex + "\n";
				return script;
			}, script);
			script += "var currentEvent = -1" + "\n";
			script += "var currentThread = -1" + "\n";
			script += "var threadStates[" + threadDatas.length + "] = [" + threadDatas.map(function(thread) { return thread.startState.index; }).join(",") + "]" + "\n";
			script += "var threadStateAges[" + threadDatas.length + "] = [" + threadDatas.map(function() { return "0"; }).join(",") + "]" + "\n";
			script += "var threadTransitions[" + threadDatas.length + "] = [" + threadDatas.map(function() { return "0"; }).join(",") + "]" + "\n";
			script += "var transitionsOld = -1" + "\n";
			script += "var transitionsNew = -1" + "\n";
			script += "timer.period[0] = 10" + "\n";
			script += "mic.threshold = 250" + "\n";
			script = transitionDatas.reduce(function(script, transition) {
				script += "\n";
				script += "sub transition" + transition.index + "Trigger" + "\n";
				script += "currentEvent = -1" + "\n";
				script += "emit transition [" + transition.index + "]" + "\n";
				script += transition.actions + "\n";
				script += "callsub state" + transition.next.index + "Enter\n";
				return script;
			}, script);
			script = stateDatas.reduce(function(script, state) {
				script += "\n";
				script += "sub state" + state.index + "Test" + "\n";
				script += "transitionsNew = 0" + "\n";
				script = state.transitions.reduce(function (script, transitionData, transitionIndex) {
					var transitionsMask = filledArray(16, "0");
					transitionsMask[transitionIndex] = "1";
					script += "if " + transitionData.condition + " then" + "\n";
					script += "transitionsNew |= 0b" + transitionsMask.join("") + "\n";
					script += "end" + "\n";
					return script;
				}, script);

				script += "sub state" + state.index + "Enter" + "\n";
				script = state.transitions.reduce(function (script, transition) {
					if (transition.events.length === 0) {
						// unconditional transition
						script += "callsub transition" + transition.index + "Trigger" + "\n";
						script += "return" + "\n";
					}
					return script;
				}, script);
				script += "threadStates[currentThread] = " + state.index + "\n";
				script += "threadStateAges[currentThread] = 0" + "\n";
				script += "callsub state" + state.index + "Test\n";
				script += "threadTransitions[currentThread] = transitionsNew" + "\n";

				return script;
			}, script);
			script = Object.keys(eventThreads).reduce(function(script, event) {
				script += "\n";
				var threads = eventThreads[event];
				script = threads.reduce(function(script, thread) {
					script += "sub " + event + thread.index + "\n";
					script += "currentThread = " + thread.index + "\n";

					script = thread.events[event].reduce(function(script, state) {
						script += "if threadStates[currentThread] == " + state.index + " then" + "\n";

						script += "transitionsOld = threadTransitions[" + thread.index + "]" + "\n";
						script += "callsub state" + state.index + "Test\n";

						script = state.events[event].reduce(function(script, transition) {
							var transitionIndex = state.transitions.indexOf(transition);
							var transitionsMask = filledArray(16, "0");
							transitionsMask[transitionIndex] = "1";
							script += "if transitionsOld & 0b" + transitionsMask.join("") + " == 0 and transitionsNew & 0b" + transitionsMask.join("") + " == 0b" + transitionsMask.join("") + " then" + "\n";
							script += "callsub transition" + transition.index + "Trigger" + "\n";
							script += "return" + "\n";
							script += "end" + "\n";
							return script;
						}, script);

						script += "threadTransitions[" + thread.index + "] = transitionsNew" + "\n";

						script += "end" + "\n";

						return script;
					}, script);
					return script;
				}, script);

				script += "onevent " + event + "\n";
				script += "currentEvent = event_" + event + "\n";
				if (event === "timer0") {
					script += "ages += [" + threads.map(function() { return "1"; }) + "]" + "\n";
				}
				script = threads.reduce(function(script, thread) {
					script += "callsub " + event + thread.index + "\n";
					return script;
				}, script);
				return script;
			}, script);

			return {
				output: {
					error: undefined,
					events: events,
					script: script,
				},
				internal: {
					threads: threadDatas,
					states: stateDatas,
					transitions: transitionDatas,
				},
			};
		}
	}

	function execReset(playing) {
		internal.states.forEach(function (state) {
			state.state.active = false;
		});
		if (playing) {
			internal.threads.forEach(function(thread) {
				internal.states[thread.startState.index].active = true;
			});
		}
	}

	function execTransition(transitionIndex) {
		internal.transitions[transitionIndex].transition.trigger();
	}

}
