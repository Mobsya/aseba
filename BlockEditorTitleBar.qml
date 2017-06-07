import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2

ToolBar {

	signal cancel()
	signal accept()

	TextMetrics {
		id: textMetrics
		font.pixelSize: 20
		font.weight: Font.Medium
		text: qsTr("Choose a block and set its parameters")
	}

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
		}
		Item {
			width: 16
		}
		Label {
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
			contentItem: Image {
				fillMode: Image.Pad
				horizontalAlignment: Image.AlignHCenter
				verticalAlignment: Image.AlignVCenter
				source: Material.theme === Material.Dark ? "qrc:/thymio-vpl2/icons/ic_done_white_24px.svg" : "qrc:/thymio-vpl2/icons/ic_done_black_24px.svg"
			}
			onClicked: accept()
		}
	}
}
