import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0

ToolBar {

	signal cancel()
	signal accept()

	TextMetrics {
		id: textMetrics
		font.pixelSize: 20
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
