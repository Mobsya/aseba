import QtQuick 2.7
import QtQuick.Controls 2.2
import ".."
import "widgets"

BlockDefinition {
	type: "action"
	category: "music"

	// FIXME: add the possibility to tell the compiler not to generate any code for this block, if no note is set

	defaultParams: {
		"speed": 10,
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

	// mapping of pitch to actual frequencies in Thymio
	readonly property var frequencyTable: [ 262, 311, 370, 440, 524, 0 ]

	function generateActionCode(params) {
		// find last non-silent note
		var noteCount = 8;
		while ((noteCount > 0) && (params.notes[noteCount-1].duration === 0))
			--noteCount;

		// if there is no note, return
		if (noteCount == 0)
			return "# zero notes in sound block";

		// generate code for notes
		var notesCopy = []
		var durationsCopy = []
		var accumulatedDuration = 0;
		for (var i = 0; i < noteCount; ++i) {
			var note = params.notes[i];
			if (note.duration === 0 && i+1 < noteCount && params.notes[i+1].duration === 0) {
				accumulatedDuration += 2 * params.speed;
			} else {
				notesCopy.push(note.duration === 0 ? 0 : frequencyTable[note.pitch]);
				var duration = note.duration === 0 ? params.speed : note.duration * params.speed;
				durationsCopy.push(duration + accumulatedDuration | 0);
				accumulatedDuration = 0;
			}
		}
		var notesCopyText = "call math.copy(notes[0:" + (notesCopy.length-1) + "], [" + notesCopy.toString() + "])\n";
		var durationsCopyText = "call math.copy(durations[0:" + (durationsCopy.length-1) + "], [" + durationsCopy.toString() + "])\n";
		return notesCopyText + durationsCopyText + "note_index = 1\nnote_count = " + notesCopy.length + "\ncall sound.freq(notes[0], durations[0])";
	}

	function compile(params) {
		return {
			action: generateActionCode(params),
			varDef:
"var notes[8]
var durations[8]
var note_index = 8
var note_count = 8
var wave[142]
var i
var wave_phase
var wave_intensity",
			initCode:
"# compute a sinus wave for sound
for i in 0:141 do
	wave_phase = (i-70)*468
	call math.cos(wave_intensity, wave_phase)
	wave[i] = wave_intensity/256
end
call sound.wave(wave)",
			eventImpl:
"# when a note is finished, play the next note
onevent sound.finished
	if note_index != note_count then
		call sound.freq(notes[note_index], durations[note_index])
		note_index += 1
	end"
		};
	}
}
