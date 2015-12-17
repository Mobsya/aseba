import QtQuick 2.0

Canvas {
	id: linkingPath

	property alias rotationAngle: linkingPathRotation.angle
	property bool trim: false

	property real leftRadius: 133
	property real rightRadius: 118

	property bool isElse: false

	Image {
		source: "images/elsePattern.png"
	}

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
		// compute start and end positions
		var localLeftRadius = leftRadius;
		var localRightRadius = rightRadius;
		var ctx = linkingPath.getContext('2d');
		if (width < 228) {
			ctx.clearRect(0, 0, width, height);
			return;
		} else if (width < 256) {
			localLeftRadius = rightRadius;
		}
		if (!trim) {
			localLeftRadius = 0;
			localRightRadius = 0;
		}
		var leftGamma = Math.acos(localLeftRadius * 0.5 / width);
		var leftArcAngle = Math.PI - 2 * leftGamma;
		var rightGamma = Math.acos(localRightRadius * 0.5 / width);
		var rightArcAngle = Math.PI - 2 * rightGamma;

		// draw the mode toggle switch
		if (width > 228 + 96) {
			ctx.fillStyle = "#f48574";
			ctx.beginPath();
			var cx = width/2;
			var cy = height*0.805;
			ctx.moveTo(cx-48, cy);
			ctx.bezierCurveTo(cx-48, cy-16, cx-24, cy-32, cx, cy-32);
			ctx.bezierCurveTo(cx+24, cy-32, cx+48, cy-16, cx+48, cy);
			ctx.fill();
		}

		// draw the arc
		ctx.lineWidth = 10;
		ctx.lineCap = "square";
		if (isElse) {
			ctx.strokeStyle = ctx.createPattern("images/elsePattern.png", "repeat");
		} else {
			ctx.strokeStyle = "#a2d8dc";
		}
		ctx.beginPath();
		ctx.arc(width/2, -Math.sqrt(width*width*3/4), width, Math.PI/3+leftArcAngle, 2*Math.PI/3-rightArcAngle);
		ctx.stroke();
	}

	onIsElseChanged: requestPaint();

	MouseArea {
		width: 96
		height: 32
		anchors.horizontalCenter: parent.horizontalCenter
		y: parent.height*0.805-32

		onClicked: {
			linkingPath.isElse = !linkingPath.isElse;
		}
	}
}
