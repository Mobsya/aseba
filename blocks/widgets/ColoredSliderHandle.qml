import QtQuick 2.6
import QtQuick.Controls.Material 2.0


Item {
	id: root
	implicitWidth: initialSize
	implicitHeight: initialSize

	property color color

	property real value: 0
	property bool handleHasFocus: false
	property bool handlePressed: false
	readonly property int initialSize: 13
	readonly property bool horizontal: control.orientation === Qt.Horizontal
	readonly property var control: parent

	Rectangle {
		id: handleRect
		width: parent.width
		height: parent.height
		radius: width / 2
		color: root.color
		scale: root.handlePressed ? 1.5 : 1

		Behavior on scale {
			NumberAnimation {
				duration: 250
			}
		}
	}
}
