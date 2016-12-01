import QtQuick 2.0
import "../.."

MouseArea {
	property bool pushed

	property url imageOn
	property url imageOff

	HDPIImage {
		anchors.fill: parent
		source: pushed ? imageOn : imageOff
	}
}
