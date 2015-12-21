import QtQuick 2.5
import ".."

BlockDefinition {
	type: "event"
	defaultParams: {}

	editor: Component {
		Item {
			id: block;

			width: 256
			height: 256
			property var params: defaultParams

			Image {
				id: button0
				source: "images/clap.svg"
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.verticalCenter: parent.verticalCenter
			}

			function getParams() {
				return {};
			}
		}
	}

	function compile(params) {
		return {
			event: "clap",
			condition: "0 == 0",
		};
	}
}
