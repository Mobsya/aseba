import QtQuick 2.0
import QtGraphicalEffects 1.0

RadialGradient {
	id: groundShadow

	property InfraredButton associatedButton

	property color gradColor: associatedButton.state == "DISABLED" ? "transparent" : (associatedButton.state == "CLOSE" ? "#ffffff" : "#000000")

	width: 200
	height: 200

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
			target: groundShadow;
			duration: 50
		}
	}
}

