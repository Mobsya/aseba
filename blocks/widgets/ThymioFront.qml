import QtQuick 2.0
import QtGraphicalEffects 1.0
import "../.."

Image {
	property color topColor: "transparent"
	property color bottomColor: "transparent"

	source: "images/thymioFront.svg"
	width: 256 // working around Qt bug with SVG and HiDPI
	height: 256 // working around Qt bug with SVG and HiDPI

	Rectangle {
		x: 33
		y: 83.6
		width: 190
		height: 30.5
		radius: 2.3
		color: "transparent"

		RadialGradient {
			anchors.fill: parent
			verticalRadius: horizontalRadius
			gradient: Gradient {
				GradientStop { position: 0; color: topColor }
				GradientStop { position: 0.25; color: topColor }
				GradientStop { position: 0.8; color: "transparent" }
			}
		}
	}

	Rectangle {
		x: 33
		y: 122.9
		width: 190
		height: 32.8
		radius: 2.3
		color: "transparent"

		RadialGradient {
			anchors.fill: parent
			verticalRadius: horizontalRadius
			gradient: Gradient {
				GradientStop { position: 0; color: bottomColor }
				GradientStop { position: 0.25; color: bottomColor }
				GradientStop { position: 0.5; color: "transparent" }
			}
		}
	}
}

