import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2

ToolBar {
	id: toolBar
	property alias okEnabled: okButton.enabled

	signal cancel()
	signal accept()

	RowLayout {
		anchors.fill: parent
		anchors.leftMargin: 4 // from: https://material.io/guidelines/layout/structure.html#structure-app-bar
		anchors.rightMargin: 4 // from: https://material.io/guidelines/layout/structure.html#structure-app-bar
		spacing: 1

		ToolButton {
			contentItem: Image {
				fillMode: Image.Pad
				horizontalAlignment: Image.AlignHCenter
				verticalAlignment: Image.AlignVCenter
				source: Material.theme === Material.Dark ? "qrc:/thymio-vpl2/icons/ic_cancel_white_24px.svg" :  "qrc:/thymio-vpl2/icons/ic_cancel_black_24px.svg"
			}
			onClicked: cancel()
			Shortcut {
				sequence: StandardKey.Cancel
				onActivated: cancel()
			}
		}
		Item {
			width: 16
		}
		Label {
			id: titleLabel
			TextMetrics {
				id: textMetrics
				font.pixelSize: titleLabel.font.pixelSize
				font.weight: titleLabel.font.weight
				text: qsTr("Choose a block and set its parameters")
			}
			text: textMetrics.width > width ? qsTr("Edit block") : textMetrics.text
			horizontalAlignment: Text.AlignHCenter
			verticalAlignment: Text.AlignVCenter
			font.pixelSize: 20
			font.weight: Font.Medium
			elide: Label.ElideRight
			Layout.fillWidth: true
		}
		Item {
			width: 16
		}
		ToolButton {
			id: okButton
			contentItem: Image {
				fillMode: Image.Pad
				horizontalAlignment: Image.AlignHCenter
				verticalAlignment: Image.AlignVCenter
				source: Material.theme === Material.Dark ? "qrc:/thymio-vpl2/icons/ic_done_white_24px.svg" : "qrc:/thymio-vpl2/icons/ic_done_black_24px.svg"
				opacity: enabled ? 1.0 : 0.5
			}
			onClicked: accept()
			Shortcut {
				sequence: "Return"
				onActivated: accept()
			}
		}
	}
}
