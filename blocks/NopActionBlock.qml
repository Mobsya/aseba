import QtQuick 2.5
import ".."

BlockDefinition {
	type: "action"

	defaultParams: null

	editor: Component {
		Image {
			property var params: defaultParams

			source: "images/nop.svg"
			width: 256 // working around Qt bug with SVG and HiDPI
			height: 256 // working around Qt bug with SVG and HiDPI

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
