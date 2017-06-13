// ekke (Ekkehard Gentz) @ekkescorner
// public domain
import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Window 2.2
import QtGraphicalEffects 1.0

Button {
	id: button
	// image should be 24x24
	property alias imageSource: contentImage.source
	// default: primaryColor
	property alias backgroundColor: buttonBackground.color
	property bool showShadow: true
	property bool isMini: Screen.width <= 460

	focusPolicy: Qt.NoFocus

	contentItem: Item {
		Image {
			id: contentImage
			anchors.centerIn: parent
			opacity: button.enabled ? 1.0 : 0.5
		}
	}
	background: Rectangle {
		id: buttonBackground
		implicitWidth: isMini ? 40 : 56
		implicitHeight: isMini ? 40 : 56
		color: button.enabled ? Material.accentColor : Qt.darker(Material.background, 1.1)
		radius: width / 2
		//opacity: button.pressed ? 0.75 : 1.0
		layer.enabled: button.showShadow
		layer.effect: DropShadow {
			verticalOffset: isMini ? 2 : 3
			horizontalOffset: isMini ? 0.7 : 1
			color: Material.dropShadowColor
			samples: button.pressed ? 20 : 10
			spread: 0.5
		}
	}
}
