import QtQuick 2.0
import QtGraphicalEffects 1.0

RadialGradient {
	id: infraredLed

	property InfraredButton associatedButton

	property color gradColor: associatedButton.state === "DISABLED" ? "transparent" : (associatedButton.state === "CLOSE" ? "#ff0000" : "#7fff0000")

	width: 24
	height: 24

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

	Behavior on gradColor {
		ColorAnimation {
			target: infraredLed;
			duration: 50
		}
	}
}

