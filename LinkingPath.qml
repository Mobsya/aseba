import QtQuick 2.0

Canvas {
	id: linkingPath

	property alias rotationAngle: linkingPathRotation.angle
	property bool trim: false

	property real leftRadius: 133
	property real rightRadius: 118 // 129

	transform: Rotation {
		id: linkingPathRotation
		origin { x: 0; y: 0 }
		angle: 0
	}

	function setLength(w) {
		width = w;
		height = w/6;
		requestPaint();
	}

	Component.onCompleted: {
		height = width/6;
		requestPaint();
	}

	onPaint: {
		var ctx = linkingPath.getContext('2d');
		ctx.lineWidth = 10;
		ctx.lineCap = "square";
		ctx.strokeStyle = "#a2d8dc";
		ctx.beginPath();
		var leftArcAngle = 0;
		var rightArcAngle = 0;
		if (trim) {
			var leftGamma = Math.acos(leftRadius * 0.5 / width);
			leftArcAngle = Math.PI - 2 * leftGamma;
			var rightGamma = Math.acos(rightRadius * 0.5 / width);
			rightArcAngle = Math.PI - 2 * rightGamma;
		}
		ctx.arc(width/2, -Math.sqrt(width*width*3/4), width, Math.PI/3+leftArcAngle, 2*Math.PI/3-rightArcAngle);
		ctx.stroke();
		//ctx.fillRect(0,0,width,width);
	}
}
