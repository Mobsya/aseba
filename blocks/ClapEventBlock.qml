import QtQuick 2.5
import ".."

BlockDefinition {
	type: "event"

	defaultParams: null

	editor: Component {
		Item {
			id: block;

			width: 256
			height: 256

			property var params: defaultParams

			Image {
				source: "images/clap.svg"
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
			condition: "0 == 0",
		};
	}
}
