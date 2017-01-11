import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0

// constants in this files are set according to Material's guidelines
// from https://material.io/guidelines/layout/structure.html#structure-app-bar

// note: according to
// - https://developer.android.com/guide/practices/screens_support.html
// - https://material.io/guidelines/layout/structure.html#structure-app-bar
// the height could change, but for now we ignore that
//height: Screen.width >= 600 ? 64 : 56 // Screen.primaryOrientation

ToolBar {
	property alias vplEditor: compilationLabel.vplEditor
	property bool isThymioConnected

	signal openDrawer()
	signal openDashelTargetSelector()

	RowLayout {
		anchors.fill: parent
		anchors.leftMargin: 4
		spacing: 1

		ToolButton {
			focusPolicy: Qt.NoFocus
			Image {
				anchors.centerIn: parent
				source: "qrc:/thymio-vpl2/icons/ic_menu_white_24px.svg"
			}
			onClicked: openDrawer()
		}

		ToolButton {
			focusPolicy: Qt.NoFocus
			Image {
				anchors.centerIn: parent
				source: isThymioConnected ? "qrc:/thymio-vpl2/icons/ic_connection_on_nonAR_white_24px.svg" : "qrc:/thymio-vpl2/icons/ic_connection_off_white_24px.svg"
			}
			onClicked: openDashelTargetSelector()
		}

		// to add spacing between buttons and text
		Item {
			width: 16
		}

		CompilationLabel {
			id: compilationLabel
		}
	}
}
