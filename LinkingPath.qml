import QtQuick 2.0

Image {
	id: linkingPath

	property alias rotationAngle: linkingPathRotation.angle

	source: "images/lineDotted.png"
	fillMode: Image.TileHorizontally

	transform: Rotation {
		id: linkingPathRotation
		origin { x: 0; y: 0 }
		angle: 0
	}
}
