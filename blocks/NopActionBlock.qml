import QtQuick 2.5
import ".."

BlockDefinition {
	type: "action"

	defaultParams: null

	editor: Component {
		Item {
			property var params: defaultParams

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
