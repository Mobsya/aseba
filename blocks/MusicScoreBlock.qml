import QtQuick 2.7
import QtQuick.Controls 2.2
import ".."
import "widgets"

BlockDefinition {
	type: "action"
	category: "music"

	readonly property MusicScore defaultScore: MusicScore {
		speed: 0.5;
		notes: [
			MusicNote { duration: 1; pitch: 1 },
			MusicNote { duration: 2; pitch: 0 },
			MusicNote { duration: 1; pitch: 1 },
			MusicNote { duration: 1; pitch: 2 },
			MusicNote { duration: 1; pitch: 3 },
			MusicNote { duration: 2; pitch: 4 },
			MusicNote { duration: 1; pitch: 3 },
			MusicNote { duration: 1; pitch: 2 }
		]
	}
	defaultParams: defaultScore

	Component {
		id: editorComponent
		Item {
			id: musicEditor
			width: 256
			height: 256

			property var params: defaultParams
			readonly property int baseX: 9
			readonly property int stepX: 30
			readonly property int baseY: 20
			readonly property int stepY: 34

			// functions
			function setNoteFromMousePos(mouse, isPress) {
				if (mouse.x < baseX || mouse.x > baseX + stepX*8)
					return;
				if (mouse.y < baseY || mouse.y > baseY + stepY*5)
					return;
				var lineIndex = (mouse.y - baseY) / stepY | 0;
				var pitch = 4 - lineIndex;
				var noteIndex = (mouse.x - baseX) / 30 | 0;
				if (params.notes[noteIndex].duration === 0 || params.notes[noteIndex].pitch === pitch) {
					if (isPress) {
						params.notes[noteIndex].duration = (params.notes[noteIndex].duration + 1) % 3;
					}
				}
				if (params.notes[noteIndex].pitch !== pitch) {
					params.notes[noteIndex].pitch = pitch;
				}
			}

			function getParams() {
				return params;
			}

			// score background
			Repeater {
				model: 5
				Rectangle {
					x: baseX
					y: baseY + index * stepY
					width: 256-18
					height: 28
					color: "#eaeaea"
					opacity: musicEditor && musicEditor.enabled ? 1.0 : 0.5
				}
			}
			// notes
			Repeater {
				model: params.notes
				Rectangle {
					visible: duration > 0
					color: duration === 1 ? "black" : "white"
					border.color: "black"
					border.width: duration === 1 ? 0 : 3
					x: baseX + index * stepX
					y: baseY + (4-pitch) * stepY
					width: 28
					height: width
					radius: width/2
				}
			}
			// notes edition
			MouseArea {
				anchors.fill: parent
				onPressed: setNoteFromMousePos( mouse, true)
				onPositionChanged: setNoteFromMousePos(mouse, false)
			}
			// speed
			Slider {
				id: speedSlider
				x: 12
				y: 200
				width: 232
				value: params.speed
				from: 0.1
				to: 1
				stepSize: 0.1
				snapMode: Slider.SnapAlways
				onValueChanged: params.speed = value
			}
		}
	}

	editor: Component {
		Loader {
			sourceComponent: editorComponent
			property var params: defaultParams
			function getParams() { return item.getParams(); }
		}
	}

	miniature: Component {
		Loader {
			sourceComponent: editorComponent
			enabled: false
			scale: 0.7
			property var params: defaultParams
			function getParams() { return item.getParams(); }
		}
	}

	function compile(params) {
		return {
			action: ""
		};
	}
}
