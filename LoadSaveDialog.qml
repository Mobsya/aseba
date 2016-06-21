import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import QtQuick.Layouts 1.3

Popup {
	// FIXME: better compute height
	id: dialog
	x: (parent.width - width) / 2
	y: (parent.height - height) / 2
	modal: true
	focus: true
	closePolicy: Popup.OnEscape | Popup.OnPressOutside

	property bool isSave
	property Editor vplEditor
	property alias programName: programName.text

	onVplEditorChanged: console.log('editor', vplEditor)

	ListModel {
		id: programList
	}

	onVisibleChanged: {
		if (visible) {
			// update list of programs
			programList.clear();
			var foundCurrent = false;
			var programs = vplEditor.listPrograms();
			for (var i = 0; i < programs.length; ++i) {
				var name = programs[i].name;
				programList.append({ "name":  name });
				if (name === programName.text)
					foundCurrent = true;
			}
			if (!foundCurrent)
				programName.text = "";
		}
	}

	ColumnLayout {
		spacing: 16

		Label {
			text: isSave ? qsTr("Save the program?") : qsTr("Load a program?")
			font.weight: Font.Medium
			font.pointSize: 21
		}

		Component {
			id: listHighlight
			Rectangle {
				color: Material.accentColor
				radius: 2
			}
		}

		ListView {
			id: list
			clip: true
			contentWidth: implicitWidth
			contentHeight: contentItem.childrenRect.height
			implicitHeight: isSave ? 100 : 150
			implicitWidth: 300
			model: programList
			delegate: ItemDelegate {
				text: name
				width: parent.width
				onClicked: {
					if (!isSave) {
						list.currentIndex = index;
					}
				}
			}
			onCurrentIndexChanged: {
				if (currentIndex !== -1) {
					programName.text = programList.get(currentIndex).name;
				}
			}

			highlight: isSave ? null : listHighlight
			ScrollIndicator.vertical: ScrollIndicator { }
		}

		TextField {
			id: programName
			anchors.left: parent.left
			anchors.right: parent.right
			visible: isSave
			focus: true
		}

		RowLayout {
			spacing: 16
			Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

			Button {
				text: qsTr("Cancel")
				onClicked: dialog.close()
			}

			Button {
				id: okButton2
				text: isSave ? qsTr("Save") : qsTr("Load")
				enabled: programName.text !== ""
				onClicked: {
					if (dialog.isSave) {
						vplEditor.saveProgram(programName.text);
					} else {
						vplEditor.loadProgram(programName.text);
					}
					dialog.close();
				}
			}
		}
	}
}
