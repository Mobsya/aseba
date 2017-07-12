import QtQuick 2.7
import QtQuick.Controls 2.2

Item {
	id: musicEditor
	width: 256
	height: 256

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
		if (notes.get(noteIndex).duration === 0 || notes.get(noteIndex).pitch === pitch) {
			if (isPress) {
				notes.set(noteIndex, { "duration": (notes.get(noteIndex).duration + 1) % 3 });
			}
		}
		if (notes.get(noteIndex).pitch !== pitch) {
			notes.set(noteIndex, { "pitch": pitch });
		}
	}

	ListModel {
		id: notes
	}
	property var params: defaultParams
	// copy from JSON to model
	onParamsChanged: {
		console.log("params changed");
		notes.clear();
		params.notes.forEach(function (note) {
			// Note: we have to recreate the array, otherwise if we extract it from the model we can a segfault
			notes.append({ "duration": note.duration, "pitch": note.pitch });
		});
	}
	// copy from model to JSON
	function getParams() {
		var newParams = {};
		newParams.speed = params.speed;
		newParams.notes = [];
		for (var i = 0; i < notes.count; ++i) {
			var note = notes.get(i);
			newParams.notes.push({ "duration": note.duration, "pitch": note.pitch })
		}
		return newParams;
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
		model: notes
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
