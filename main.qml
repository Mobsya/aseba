import QtQuick 2.6
import QtQuick.Window 2.2
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import Qt.labs.settings 1.0
import "qrc:/thymio-vpl2"

ApplicationWindow {
	id: window
	title: qsTr("Thymio Flow")
	visibility: Qt.platform.os === "android" ? Window.FullScreen : Window.Windowed
	visible: true
	width: 960
	height: 600

	header: ToolBar {
		RowLayout {
			anchors.fill: parent

			ToolButton {
				contentItem: Image {
					anchors.centerIn: parent
					source: "qrc:/thymio-vpl2/icons/ic_menu_white_24px.svg"
				}
				onClicked: drawer.open()
			}

			ToolButton {
				contentItem: Image {
					anchors.centerIn: parent
					source: !!thymio.node ? "qrc:/thymio-vpl2/icons/ic_connection_on_white_24px.svg" : "qrc:/thymio-vpl2/icons/ic_connection_off_white_24px.svg"
				}
				onClicked: dashelTargetSelector.open()
			}

			CompilationLabel {
				vplEditor: vplEditor
			}

			ToolButton {
				contentItem: Image {
					anchors.centerIn: parent
					source: !thymio.playing ? "qrc:/thymio-vpl2/icons/ic_play_white_48px.svg" : "qrc:/thymio-vpl2/icons/ic_stop_white_48px.svg"
				}
				enabled: (vplEditor.compiler.error === "") && (thymio.node !== undefined)
				onClicked: thymio.playing = !thymio.playing
			}
		}
	}

	Editor {
		id: vplEditor
		anchors.fill: parent

		Text {
			text: "developer preview pre-alpha, no feature or design is final"
			anchors.left: parent.left
			anchors.leftMargin: 106
			anchors.top: parent.top
			anchors.topMargin: 10
			color: Material.primaryTextColor
		}
	}

	ListModel {
		id: menuItems

		ListElement { title: qsTr("load program"); save: false; icon: "qrc:/thymio-vpl2/icons/ic_open_white_24px.svg" }
		ListElement { title: qsTr("save program"); save: true; icon: "qrc:/thymio-vpl2/icons/ic_save_white_24px.svg" }
		ListElement { title: qsTr("new program"); newProgram: true; icon: "qrc:/thymio-vpl2/icons/ic_new_white_24px.svg" }
		//ListElement { title: qsTr("about"); source: "About.qml" ; icon: "qrc:/thymio-vpl2/icons/ic_info_white_24px.svg" }
	}

	Drawer {
		id: drawer
		edge: Qt.LeftEdge
		position: 0
		width: 280
		height: window.height

		contentItem: Pane {
			ListView {
				id: listView
				currentIndex: -1
				anchors.fill: parent

				model: menuItems

				delegate: ItemDelegate {
					contentItem: Row {
						spacing: 24
						HDPIImage {
							source: icon
							width: 24
							height: 24
							opacity: enabled ? 1.0 : 0.5
						}
						Text {
							text: model.title
							font.pixelSize: 14
							font.weight: Font.Medium
							color: Material.primaryTextColor
							opacity: enabled ? 1.0 : 0.5
						}
					}
					text: model.title
					highlighted: ListView.isCurrentItem
					onClicked: {
						if (newProgram === true) {
							// new program
							vplEditor.clearProgram();
							saveProgramDialog.programName = "";
						} else {
							// load/save dialog
							saveProgramDialog.isSave = save;
							saveProgramDialog.visible = true;
						}
						drawer.close()
					}
				}
			}
		}
	}

	DashelTargetDialog {
		id: dashelTargetSelector
		aseba: aseba
	}

	LoadSaveDialog {
		id: saveProgramDialog
		vplEditor: vplEditor
	}

	Aseba {
		id: aseba
		onTargetChanged: console.log("target", target)
		onUserMessage: {
			if (type !== 0) {
				return;
			}
			if (vplEditor === undefined) {
				return;
			}
			vplEditor.compiler.execTransition(data[0], data[1]);
		}
	}

	Thymio {
		id: thymio
		program: playing ? vplEditor.compiler.source : ""
		onNodeChanged: playing = false
		onPlayingChanged: vplEditor.compiler.execReset(playing);
	}
}

