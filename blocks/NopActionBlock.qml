import QtQuick 2.5
import ".."

BlockDefinition {
	type: "action"

	defaultParams: null

	editor: Component {
		Image {
			property var params: defaultParams

			source: "images/nop.svg"

			function getParams() {
				return null;
			}
		}
	}

	function compile(params) {
		return {
			action: ""
		};
	}
}
