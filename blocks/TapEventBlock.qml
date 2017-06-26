import QtQuick 2.7
import ".."

BlockDefinition {
	type: "event"
	category: "tapClap"

	defaultParams: null

	editor: Component {
		Item {
			id: block;

			width: 256
			height: 256

			property var params: defaultParams

			Image {
				source: "images/tap.svg"
				width: 256 // working around Qt bug with SVG and HiDPI
				height: 256 // working around Qt bug with SVG and HiDPI
				anchors.centerIn: parent
			}

			function getParams() {
				return null;
			}
		}
	}

	function compile(params) {
		return {
			event: "tap",
			condition: "currentEvent == event_tap",
		};
	}
}
