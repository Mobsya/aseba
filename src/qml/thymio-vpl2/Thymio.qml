import QtQuick 2.7
import Thymio 1.0

Item {
    property var ready: node && node.ready
    property var node
    property var variables: ({})
    property var program: ({
        "events": {},
        "source": "",
    })
    property string error: ""

    onNodeChanged: {
        if (node) {
            setVariables();
            setProgram();
        }
    }
    onReadyChanged: {
        if(ready) {
            setVariables();
            setProgram();
        }
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

    function setVariable(name, value) {
        if (node) {
            if (typeof value === "number") {
                value = [value];
            }
            node.setVariable(name, value);
        }
    }

    function setProgram() {
        if (node) {
            error = node.setProgram(program.events, program.source);
        }
    }
}
