import QtQuick 2.7
import ".."
import "widgets"

BlockDefinition {
	type: "action"
	category: "motor"

	defaultParams: "stop"

	function compile(params) {
		function getTargets() {
			switch(params) {
			case "frontLeft": return [0, 250];
			case "front": return [250, 250];
			case "frontRight": return [250, 0];
			case "right": return [250, -250];
			case "backRight": return [-250, 0];
			case "back": return [-250, -250];
			case "backLeft": return [0, -250];
			case "left": return [-250, 250];
			case "stop": return [0, 0];
			}
		}

		var targets = getTargets();
		var leftTarget = targets[0];
		var rightTarget = targets[1];
		return {
			action: "motor.left.target = " + leftTarget + "\n"
					+ "motor.right.target = " + rightTarget,
		};
	}

	editor: Component {
		Item {
			id: block

			scale: 0.8
			width: 256
			height: 256

			property alias params: block.state
			function getParams() {
				return state;
			}

			PushButton {
				x: 29.2
				y: 28
				width: 51.6
				height: 51.6
				pushed: parent.state === "frontLeft"
				onClicked: parent.state = "frontLeft"
				imageOn: "MotorSimpleActionBlock/edit-frontLeft-on.svg"
				imageOff: "MotorSimpleActionBlock/edit-frontLeft-off.svg"
			}

			PushButton {
				x: 102.2
				y: 6.3
				width: 51.6
				height: 51.6
				pushed: parent.state === "front"
				onClicked: parent.state = "front"
				imageOn: "MotorSimpleActionBlock/edit-front-on.svg"
				imageOff: "MotorSimpleActionBlock/edit-front-off.svg"
			}

			PushButton {
				x: 175.2
				y: 28
				width: 51.6
				height: 51.6
				pushed: parent.state === "frontRight"
				onClicked: parent.state = "frontRight"
				imageOn: "MotorSimpleActionBlock/edit-frontRight-on.svg"
				imageOff: "MotorSimpleActionBlock/edit-frontRight-off.svg"
			}

			PushButton {
				x: 197.6
				y: 101
				width: 51.6
				height: 51.6
				pushed: parent.state === "right"
				onClicked: parent.state = "right"
				imageOn: "MotorSimpleActionBlock/edit-right-on.svg"
				imageOff: "MotorSimpleActionBlock/edit-right-off.svg"
			}

			PushButton {
				x: 175.2
				y: 173.9
				width: 51.6
				height: 51.6
				pushed: parent.state === "backRight"
				onClicked: parent.state = "backRight"
				imageOn: "MotorSimpleActionBlock/edit-backRight-on.svg"
				imageOff: "MotorSimpleActionBlock/edit-backRight-off.svg"
			}

			PushButton {
				x: 102.2
				y: 195.6
				width: 51.6
				height: 51.6
				pushed: parent.state === "back"
				onClicked: parent.state = "back"
				imageOn: "MotorSimpleActionBlock/edit-back-on.svg"
				imageOff: "MotorSimpleActionBlock/edit-back-off.svg"
			}

			PushButton {
				x: 29.2
				y: 173.9
				width: 51.6
				height: 51.6
				pushed: parent.state === "backLeft"
				onClicked: parent.state = "backLeft"
				imageOn: "MotorSimpleActionBlock/edit-backLeft-on.svg"
				imageOff: "MotorSimpleActionBlock/edit-backLeft-off.svg"
			}

			PushButton {
				x: 6.8
				y: 101
				width: 51.6
				height: 51.6
				pushed: parent.state === "left"
				onClicked: parent.state = "left"
				imageOn: "MotorSimpleActionBlock/edit-left-on.svg"
				imageOff: "MotorSimpleActionBlock/edit-left-off.svg"
			}

			PushButton {
				x: 82.2
				y: 81.2
				width: 91.6
				height: 91.5
				pushed: parent.state === "stop"
				onClicked: parent.state = "stop"
				imageOn: "MotorSimpleActionBlock/edit-stop-on.svg"
				imageOff: "MotorSimpleActionBlock/edit-stop-off.svg"
			}
		}
	}

	miniature: Component {
		HDPIImage {
			property string params
			width: 256
			height: 256
			source: {
				switch(params) {
				case "frontLeft": return "MotorSimpleActionBlock/mini-frontLeft.svg";
				case "front": return "MotorSimpleActionBlock/mini-front.svg";
				case "frontRight": return "MotorSimpleActionBlock/mini-frontRight.svg";
				case "right": return "MotorSimpleActionBlock/mini-right.svg";
				case "backRight": return "MotorSimpleActionBlock/mini-backRight.svg";
				case "back": return "MotorSimpleActionBlock/mini-back.svg";
				case "backLeft": return "MotorSimpleActionBlock/mini-backLeft.svg";
				case "left": return "MotorSimpleActionBlock/mini-left.svg";
				case "stop": return "MotorSimpleActionBlock/mini-stop.svg";
				default: return "MotorSimpleActionBlock/mini.svg";
				}
			}
		}
	}
}
