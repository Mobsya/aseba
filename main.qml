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

	Material.primary: Material.theme === Material.Dark ? "#200032" : Material.Indigo
	Material.accent: Material.theme === Material.Dark ? "#9478aa" : Material.Pink

	header: ToolBar {
		RowLayout {
			anchors.fill: parent

			ToolButton {
				contentItem: Image {
					anchors.centerIn: parent
					source: "qrc:/thymio-vpl2/icons/ic_menu_white_24px.svg"
				}
				visible: !vplEditor.blockEditorVisible
				onClicked: drawer.open()
			}

			ToolButton {
				contentItem: Image {
					anchors.centerIn: parent
					source: !!thymio.node ? "qrc:/thymio-vpl2/icons/ic_connection_on_white_24px.svg" : "qrc:/thymio-vpl2/icons/ic_connection_off_white_24px.svg"
				}
				visible: !vplEditor.blockEditorVisible
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
				visible: !vplEditor.blockEditorVisible
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

	Connections {
		target: vplEditor.compiler
		onSourceChanged: thymio.playing = false
	}

	ListModel {
		id: menuItems

		ListElement { title: qsTr("load program"); save: false; whiteIcon: "qrc:/thymio-vpl2/icons/ic_open_white_24px.svg"; blackIcon: "qrc:/thymio-vpl2/icons/ic_open_black_24px.svg"; }
		ListElement { title: qsTr("save program"); save: true; whiteIcon: "qrc:/thymio-vpl2/icons/ic_save_white_24px.svg"; blackIcon: "qrc:/thymio-vpl2/icons/ic_save_black_24px.svg"; }
		ListElement { title: qsTr("new program"); newProgram: true; whiteIcon: "qrc:/thymio-vpl2/icons/ic_new_white_24px.svg"; blackIcon: "qrc:/thymio-vpl2/icons/ic_new_black_24px.svg";}
		ListElement { title: qsTr("switch color theme"); switchColorTheme: true; whiteIcon: ""; blackIcon: ""; }
		//ListElement { title: qsTr("about"); source: "About.qml" ; icon: "qrc:/thymio-vpl2/icons/ic_info_white_24px.svg" }
	}

	Drawer {
		id: drawer
		edge: Qt.LeftEdge
		position: 0
		width: 260
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
							source: Material.theme === Material.Dark ? whiteIcon : blackIcon
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
						} else if (switchColorTheme === true) {
							if (window.Material.theme === Material.Dark) {
								window.Material.theme = Material.Light;
							} else {
								window.Material.theme = Material.Dark;
							}
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
		property bool playing: false
		events: vplEditor ? vplEditor.compiler.events : {}
		source: playing ? vplEditor.compiler.source : ""
		onNodeChanged: playing = false
		onPlayingChanged: vplEditor.compiler.execReset(playing);
	}
}

