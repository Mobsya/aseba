import QtQuick 2.7

Timer {
	interval: 17
	repeat: true

	property real vx: 0 // in px per second
	property real vy: 0 // in px per second
	property var prevMousePos // last mouse position in scene coordinates
	property double prevTime: 0 // from Date().valueOf()

	onTriggered: {
		parent.x += (vx * interval) * 0.001;
		parent.y += (vy * interval) * 0.001;
		vx *= 0.85;
		vy *= 0.85;
		if (Math.abs(vx) < 1 && Math.abs(vy) < 1) {
			running = false;
			vx = 0;
			vy = 0;
		}
	}

	function startEstimation(mousePos) {
		var now = new Date().valueOf();
		vx = 0;
		vy = 0;
		prevMousePos = mousePos;
		prevTime = now;
	}

	function updateEstimation(mousePos) {
		var now = new Date().valueOf();
		var dx = mousePos.x - prevMousePos.x;
		var dy = mousePos.y - prevMousePos.y;
		var dt = now - prevTime;
		vx = vx * 0.6 + dx * 0.4 * dt;
		vy = vy * 0.6 + dy * 0.4 * dt;
		prevMousePos = mousePos;
		prevTime = now;
	}

	function startAcceleration() {
		running = true;
	}
}
