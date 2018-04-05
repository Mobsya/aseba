import QtQuick 2.7
import Thymio 1.0

Item {
    property var node /*{
        for (var i = 0; i < aseba.nodes.length; ++i) {
            var node = aseba.nodes[i];
            if (node.name === "thymio-II") {
                return node;
            }
        }
    }*/

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
        /*
        // uncomment this section for simulating the execution at each run
        // TODO: put simulation in a thread
        // TODO: only trigger this when play is pressed, not when stop is
        var scenario = {
            duration: 10,
            worldSize: Qt.vector2d(100, 100),
            thymio : { position: Qt.vector2d(20, 50), angle: 0 },
            walls: [ { position: Qt.vector2d(80, 50), angle: Math.pi / 2, size: Qt.vector3d(20, 2, 10), color: "Blue" } ],
            groundTexture: "/tmp/test.png"
        }
        var simError = simulator.runProgram(scenario, program.events, program.source, function() {
            if (this.currentTime === 0) {
                this.simulatedThymio.tap();
            }
            console.log(this.currentTime + ": " + this.simulatedThymio.position.x + " : " + this.simulatedThymio.horizontalSensors[2]);
        });
        if (simError)
            console.log("Simulation error: " + simError)
        */
        if (node) {
            error = node.setProgram(program.events, program.source);
        }
    }
}
