import QtQuick 2.0

Canvas {
	id: linkingPath

	property alias rotationAngle: linkingPathRotation.angle
	property bool trim: false

	transform: Rotation {
		id: linkingPathRotation
		origin { x: 0; y: 0 }
		angle: 0
	}

	function setLength(w) {
		width = w;
		height = w;
		requestPaint();
	}

	Component.onCompleted: {
		height = width;
		requestPaint();
	}

	onPaint: {
		var ctx = linkingPath.getContext('2d');
		ctx.lineWidth = 10;
		ctx.lineCap = "square";
		ctx.strokeStyle = "#eceded";
		ctx.beginPath();
		var arcAngle = 0;
		if (trim) {
			var centerRadius = 99;
			var gamma = Math.acos(centerRadius * 0.5 / width);
			arcAngle = Math.PI - 2 * gamma;
		}
		ctx.arc(width/2, -Math.sqrt(width*width*3/4), width, Math.PI/3+arcAngle, 2*Math.PI/3-arcAngle);
		ctx.stroke();
		//ctx.fillRect(0,0,width,width);
	}
}
