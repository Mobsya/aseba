import QtQuick 2.7
import ".."
import "widgets"

BlockDefinition {
	type: "action"
	category: "motor"

	defaultParams: [ 0, 0 ]

	editor: Component {
		MotorSliders {
			params: defaultParams
			miniature: false
		}
	}

	miniature: Component {
		MotorSliders {
			params: defaultParams
			miniature: true
		}
	}

	function compile(params) {
		return {
			action: "motor.left.target = " + (params[0] | 0) + "\n" + "motor.right.target = " + (params[1] | 0)
		};
	}
}
