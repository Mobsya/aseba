import QtQuick 2.0

Timer {
	property bool highlighted: false
	interval: 100
	onTriggered: highlighted = false;
	function highlight() {
		highlighted = true;
		restart();
	}
}
