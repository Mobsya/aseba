import QtQuick 2.0
import QtGraphicalEffects 1.0

RadialGradient {
	id: infraredLed

	property InfraredButton associatedButton

	property color gradColor: "transparent"

	width: 24
	height: 24
	state: associatedButton.state
	gradient: Gradient {
		GradientStop {
			position: 0.0;
			color: gradColor;
		}
		GradientStop {
			position: 0.5;
			color: "transparent";
		}
	}

	states: [
		State {
			name: "CLOSE"
			PropertyChanges { target: infraredLed; gradColor: "#ff0000"; }
		},
		State {
			name: "FAR"
			PropertyChanges { target: infraredLed; gradColor: "#7fff0000"; }
		}
	]

	transitions:
		Transition {
			to: "*"
			ColorAnimation { target: infraredLed; duration: 30}
		}
}

