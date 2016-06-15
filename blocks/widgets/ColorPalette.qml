import QtQuick 2.0

Canvas {
	width: 192
	height: 96

	function bodyValue(value) {
		return (value*175+80) / 255.;
	}

	property color color
	property color bodyColor: Qt.rgba(bodyValue(color.r), bodyValue(color.g), bodyValue(color.b), Math.max(bodyValue(color.r), bodyValue(color.g), bodyValue(color.b)))

	onPaint: {
		var ctx = getContext("2d");
		var h, l;
		for (h = 0; h < 32; ++h) {
			for (l = 0; l < 16; ++l) {
				ctx.fillStyle = Qt.hsla(h / 32.0, 1.0, 1.0 - l / 15.0, 1.0);
				ctx.fillRect(h*6, l*6, 6, 6);
			}
		}
	}

	MouseArea {
		anchors.fill: parent

		function setColorFromMousePos(mouse) {
			var h = mouse.x / 6;
			var l = mouse.y / 6;
			color = Qt.hsla(h / 32.0, 1.0, 1.0 - l / 15.0, 1.0);
		}

		onClicked: setColorFromMousePos(mouse);
		onPositionChanged: setColorFromMousePos(mouse);
	}
}
