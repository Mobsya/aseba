import QtQuick 2.0
import Aseba 1.0

Item {
	property var variables: ({})
	property string program: ""
	property bool playing: false

	property var node: {
		for (var i = 0; i < aseba.nodes.length; ++i) {
			var node = aseba.nodes[i];
			if (node.name === "thymio-II") {
				return node;
			}
		}
	}

	onNodeChanged: {
		setVariables();
		setProgram();
	}
	onVariablesChanged: setVariables()
	onProgramChanged: setProgram()

	function setVariables() {
		if (node) {
			Object.keys(variables).forEach(function(name) {
				var value = variables[name];
				if (typeof value === "number") {
					value = [value];
				}
				node.setVariable(name, value);
			})
		}
	}

	function setProgram() {
		if (node) {
			node.setProgram(program);
		}
	}
}
