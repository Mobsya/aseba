import QtQuick 2.5
import QtQuick.Window 2.2
import "qrc:/thymio-vpl2"

Window {
	visible: true
	width: 1280
	height: 720
	color: "white"

	FullEditor {
		anchors.fill: parent
	}
}

