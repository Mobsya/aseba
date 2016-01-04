import QtQuick 2.0
import QtGraphicalEffects 1.0

Image {
	property color topColor: "#ffffff"
	property color bottomColor: "#ffffff"

	source: "images/thymioFront.svg"

	Rectangle {
		x: 33
		y: 83.6
		width: 190
		height: 30.5
		radius: 2.3

		RadialGradient {
			anchors.fill: parent
			verticalRadius: horizontalRadius
			gradient: Gradient {
				GradientStop { position: 0; color: topColor }
				GradientStop { position: 0.5; color: "transparent" }
			}
		}
		visible: topColor != "#ffffff"
	}

	Rectangle {
		x: 33
		y: 122.9
		width: 190
		height: 32.8
		radius: 2.3

		RadialGradient {
			anchors.fill: parent
			verticalRadius: horizontalRadius
			gradient: Gradient {
				GradientStop { position: 0; color: bottomColor }
				GradientStop { position: 0.5; color: "transparent" }
			}
		}
		visible: bottomColor != "#ffffff"
	}
}

