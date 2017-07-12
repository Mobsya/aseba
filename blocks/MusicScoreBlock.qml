import QtQuick 2.7
import QtQuick.Controls 2.2
import ".."
import "widgets"

BlockDefinition {
	type: "action"
	category: "music"

	defaultParams: {
		"speed": 0.5,
		"notes": [
			{ "duration": 1, "pitch": 1 },
			{ "duration": 2, "pitch": 0 },
			{ "duration": 1, "pitch": 1 },
			{ "duration": 1, "pitch": 2 },
			{ "duration": 1, "pitch": 3 },
			{ "duration": 2, "pitch": 4 },
			{ "duration": 1, "pitch": 3 },
			{ "duration": 1, "pitch": 2 }
		]
	}

	editor: Component {
		MusicScoreEditor {
			params: defaultParams
		}
	}

	miniature: Component {
		MusicScoreEditor {
			enabled: false
			scale: 0.7
			params: defaultParams
		}
	}

	function compile(params) {
		return {
			action: ""
		};
	}
}
