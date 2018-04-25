import QtQuick 2.7
import ".."

BlockDefinition {
	type: "action"
	category: "nop"

	defaultParams: null

	editor: Component {
		HDPIImage {
			property var params: defaultParams

			source: "NopActionBlock.svg"
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
