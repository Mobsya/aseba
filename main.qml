import QtQuick 2.7
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import Qt.labs.settings 1.0
import QtQml 2.2
import "qrc:/thymio-vpl2"

ApplicationWindow {
	id: window
	title: qsTr("Thymio VPL Mobile Preview")
	visible: true
	width: 960
	height: 600

	Material.primary: Material.theme === Material.Dark ? "#200032" : Material.background // "#a3d9db"
	Material.accent: Material.theme === Material.Dark ? "#9478aa" : "#B290CC" // "#59cbc8"
	Material.background: Material.theme === Material.Dark ? "#ff44285a" : "white"

	header: vplEditor.blockEditorVisible ? blockEditorTitleBar : vplEditorTitleBar

	readonly property string autosaveName: qsTr("autosave");

	EditorTitleBar {
		id: vplEditorTitleBar
		visible: !vplEditor.blockEditorVisible
		vplEditor: vplEditor
		isThymioConnected: !!thymio.node
		onOpenDrawer: drawer.open()
		onOpenDashelTargetSelector: dashelTargetSelector.open()
	}

	BlockEditorTitleBar {
		id: blockEditorTitleBar
		visible: vplEditor.blockEditorVisible
		onCancel: vplEditor.blockEditor.close()
		onAccept: vplEditor.blockEditor.accept()
	}

	Editor {
		id: vplEditor
		anchors.fill: parent

//		Text {
//			text: "developer preview pre-alpha, no feature or design is final"
//			anchors.left: parent.left
//			anchors.leftMargin: 106
//			anchors.top: parent.top
//			anchors.topMargin: 10
//			color: Material.primaryTextColor
//		}
	}

	// improve using: https://appbus.wordpress.com/2016/05/20/one-page-sample-app/
	FloatingActionButton {
		property int distToBorders: isMini ? 16 : 24
		anchors.right: parent.right
		anchors.rightMargin: (isMini ? 64 : 96) + distToBorders
		anchors.bottom: parent.bottom
		anchors.bottomMargin: distToBorders
		isMini: window.width <= 460
		/*anchors.top: parent.top
		anchors.topMargin: -height/2
		z: 2*/
		imageSource: !thymio.playing ? "qrc:/thymio-vpl2/icons/ic_play_arrow_white_24px.svg" : "qrc:/thymio-vpl2/icons/ic_stop_white_24px.svg"
		visible: !vplEditor.blockEditorVisible
		onClicked: thymio.playing = !thymio.playing
		enabled: (vplEditor.compiler.output.error === undefined) && (thymio.node !== undefined)
		//opacity: enabled ? 1.0 : 0.38
	}

	Connections {
		target: vplEditor.compiler
		onOutputChanged: thymio.playing = false
	}

	ListModel {
		id: menuItems

		ListElement {
			title: QT_TR_NOOP("load program");
			callback: "load";
			whiteIcon: "qrc:/thymio-vpl2/icons/ic_open_white_24px.svg";
			blackIcon: "qrc:/thymio-vpl2/icons/ic_open_black_24px.svg";
		}
		function load() {
			saveProgramDialog.isSave = false;
			saveProgramDialog.visible = true;
		}

		ListElement {
			title: QT_TR_NOOP("save program");
			callback: "save";
			whiteIcon: "qrc:/thymio-vpl2/icons/ic_save_white_24px.svg";
			blackIcon: "qrc:/thymio-vpl2/icons/ic_save_black_24px.svg";
		}
		function save() {
			saveProgramDialog.isSave = true;
			saveProgramDialog.visible = true;
		}

		ListElement {
			title: QT_TR_NOOP("new program");
			callback: "newProgram";
			whiteIcon: "qrc:/thymio-vpl2/icons/ic_new_white_24px.svg";
			blackIcon: "qrc:/thymio-vpl2/icons/ic_new_black_24px.svg";
		}
		function newProgram() {
			vplEditor.clearProgram();
			saveProgramDialog.programName = "";
		}

		ListElement {
			title: QT_TR_NOOP("switch color theme");
			callback: "switchColorTheme";
			whiteIcon: "qrc:/thymio-vpl2/icons/ic_invert_colors_white_24px.svg";
			blackIcon: "qrc:/thymio-vpl2/icons/ic_invert_colors_black_24px.svg";
		}
		function switchColorTheme() {
			if (window.Material.theme === Material.Dark) {
				window.Material.theme = Material.Light;
			} else {
				window.Material.theme = Material.Dark;
			}
		}

		/*ListElement {
			title: QT_TR_NOOP("switch editor mode");
			callback: "switchEditorMode";
			whiteIcon: "";
			blackIcon: "";
		}
		function switchEditorMode() {
			vplEditor.switchMode();
		}*/

		ListElement {
			title: QT_TR_NOOP("about");
			callback: "showAboutBox";
			whiteIcon: "qrc:/thymio-vpl2/icons/ic_info_white_24px.svg";
			blackIcon: "qrc:/thymio-vpl2/icons/ic_info_black_24px.svg";
		}
		function showAboutBox() {
			aboutDialog.visible = true;
		}

		ListElement {
			title: "dev: show aesl";
			callback: "showAeslSource";
			whiteIcon: "";
			blackIcon: "";
			visible: false;
		}
		function showAeslSource() {
			aeslSourceDialog.visible = true;
		}
	}

	Drawer {
		id: drawer
		edge: Qt.LeftEdge
		position: 0
		width: 300
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
							text: qsTr(model.title);
							font.pixelSize: 14
							font.weight: Font.Medium
							color: Material.primaryTextColor
							opacity: enabled ? 1.0 : 0.5
							visible: ((model.title.substring(0, 4) !== "dev:") || (Qt.application.arguments.indexOf("--developer") !== -1))
						}
					}
					onClicked: {
						ListView.view.model[callback]();
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

	Popup {
		id: aboutDialog
		x: (parent.width - width) / 2
		y: (parent.height - height) / 2
		modal: true
		padding: 24

		Column {
			spacing: 20
			padding: 0
			Label {
				text: qsTr("Thymio VPL Mobile Preview")
				font.pixelSize: 20
				font.weight: Font.Medium
			}
			Text {
				text:
					qsTr("<p><a href=\"http://stephane.magnenat.net\">Stéphane Magnenat</a>, <a href=\"http://sampla.ch\">Martin Voelkle</a><br/>and <a href=\"http://mariamari-a.com\">Maria Beltran</a></p>") +
					qsTr("<p>(c) 2015–2017 EPFL and ETH Zürich</p>") +
					qsTr("<p>This project is open source under <a href=\"https://github.com/aseba-community/thymio-vpl2/blob/master/LICENSE.txt\">LGPL</a></p>") +
					qsTr("<p>See file <a href=\"https://github.com/aseba-community/thymio-vpl2/blob/master/AUTHORS.md\">AUTHORS.md</a> in the <a href=\"https://github.com/aseba-community/thymio-vpl2\">source code</a><br/>") +
					qsTr("for a detailed list of contributions.</p>")
				font.pixelSize: 14
				font.weight: Font.Normal
				lineHeight: 20
				lineHeightMode: Text.FixedHeight
				onLinkActivated: Qt.openUrlExternally(link)
			}
		}
	}

	// developer options for debugging
	Popup {
		id: aeslSourceDialog
		x: (parent.width - width) / 2
		y: (parent.height - height) / 2
		width: 0.8 * parent.width
		height: 0.8 * parent.height
		modal: true
		focus: true

		Flickable {
			anchors.fill: parent
			clip: true
			Text {
				text: prettyPrintGeneratedAesl(vplEditor.compiler.output.script)
				color: Material.primaryTextColor
				font.family: "Monospace"
				// TODO: move this somewhere
				function prettyPrintGeneratedAesl(source) {
					var level = 0;
					var output = "";
					var splitted = source.split("\n");
					for (var i = 0; i < splitted.length; i++) {
						var line = splitted[i].trim();
						if ((line.indexOf("sub ") === 0) || (line.indexOf("onevent ") === 0)) {
							output += "\n" + line + "\n";
							level = 1;
						} else {
							if (line.indexOf("end") === 0) {
								level -= 1;
							}
							for (var j = 0; j < level; j++)
								output += "    ";
							output += line + "\n";
							if (line.indexOf("if ") === 0) {
								level += 1;
							}
						}
					}
					return output;
				}
			}
			contentWidth: contentItem.childrenRect.width;
			contentHeight: contentItem.childrenRect.height
			ScrollBar.vertical: ScrollBar { }
			ScrollBar.horizontal: ScrollBar { }
		}
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
			vplEditor.compiler.execTransition(data[0]);
		}
	}

	Thymio {
		id: thymio
		property bool playing: false
		events: vplEditor ? vplEditor.compiler.output.events : {}
		source: playing ? vplEditor.compiler.output.script : ""
		onNodeChanged: playing = false
		onPlayingChanged: {
			vplEditor.compiler.execReset(playing);
			if (!playing) {
				setVariable("motor.left.target", 0);
				setVariable("motor.right.target", 0);
			} else {
				vplEditor.saveProgram(autosaveName);
			}
		}
		onErrorChanged: if (error !== "") { vplEditor.compiler.output.error = error; }
	}

	Component.onCompleted: {
		vplEditor.loadProgram(autosaveName);
	}
}

