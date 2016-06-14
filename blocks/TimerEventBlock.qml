import QtQuick 2.5
import QtQuick.Controls 2.0
//import QtQuick.Controls 1.4
//import QtQuick.Controls.Styles 1.4
import ".."
import "widgets"

BlockDefinition {
	type: "event"

	defaultParams: 100

	function paramsToMajorValue(params) {
		return (params >= 1000) ? 2 : (params >= 100 ? 1 : 0);
	}

	function paramsToMinorValue(params) {
		return (params >= 1000) ? (params / 1000 | 0) : (params >= 100 ? (params / 100 | 0) : (params / 10 | 0));
	}

	editor: Component {
		Item {
			width: 256
			height: 256
			property var params: defaultParams

			Text {
				id: label
				anchors.horizontalCenter: parent.horizontalCenter
				y: 4
				color: "white"
			}

			Slider {
				id: majorSlider

				x: 84
				y: 220
				width: 88
				height: 32

				from: 0
				to: 2
				stepSize: 1
				snapMode: Slider.SnapAlways

				value: paramsToMajorValue(params)

				onValueChanged: updateLabel()
			}

			Slider {
				id: minorSlider

				x: 200
				y: 37
				width: 32
				height: 170
				orientation: Qt.Vertical

				from: 1
				to: 9
				stepSize: 1
				snapMode: Slider.SnapAlways

				value: paramsToMinorValue(params)

				onValueChanged: updateLabel()
			}

			TimerGlass {
				x: 0
				y: -6
				majorValue: majorSlider.value
				minorValue: minorSlider.value
			}

			function updateLabel() {
				var duration = getParams();
				if (duration < 100) {
					label.text = "." + ((duration / 10) | 0).toString() + " s";
				} else {
					label.text = ((duration / 100) | 0).toString() + " s";
				}
			}

			function getParams() {
				switch(majorSlider.value) {
				case 0:
					return 10 * minorSlider.value;
				case 1:
					return 100 * minorSlider.value;
				default:
					return 1000 * minorSlider.value;
				}
			}
		}
	}

	miniature: Component {
		Item {
			width: 256
			height: 256
			property var params: defaultParams

			TimerGlass {
				anchors.centerIn: parent
				majorValue: paramsToMajorValue(params)
				minorValue: paramsToMinorValue(params)
			}
		}
	}

	function compile(param) {
		return {
			event: "timer0",
			condition: "ages[thread] >= " + param,
		};
	}
}
