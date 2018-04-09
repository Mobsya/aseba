import QtQuick 2.7
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtGraphicalEffects 1.0
import Qt.labs.settings 1.0
import QtQml 2.2
import Thymio 1.0
import "."

Rectangle {
    id: window
    //title: qsTr("Thymio VPL Mobile Preview")
    visible: true
  //  width: 960
  //  height: 600

    Material.primary: Material.theme === Material.Dark ? "#200032" : Material.background // "#a3d9db"
    property string linkRichTextStyle: Material.theme === Material.Dark ?
        "<style>a:link { text-decoration: none; color: \"#90CAF9\" }</style>" :
        "<style>a:link { text-decoration: none; color: \"#3F51B5\" }</style>"
    Material.accent: Material.theme === Material.Dark ? "#9478aa" : "#B290CC" // "#59cbc8"
    Material.background: Material.theme === Material.Dark ? "#ff44285a" : "white"

    readonly property string autosaveName: qsTr("autosave");


    Item {
        z  :10000
        id : header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top  : parent.top
        height: vplEditorTitleBar.height

        EditorTitleBar {
            id: vplEditorTitleBar
            visible: !vplEditor.blockEditorVisible
            vplEditor: vplEditor
            isThymioConnected: !!thymio.node
            onOpenDrawer: drawer.open()
            onOpenDashelTargetSelector: dashelTargetSelector.open()
            anchors.fill: parent
        }

        BlockEditorTitleBar {
            id: blockEditorTitleBar
            visible: vplEditor.blockEditorVisible
            onCancel: vplEditor.blockEditor.close()
            onAccept: vplEditor.blockEditor.accept()
            okEnabled: vplEditor.blockEditorDefinition
            anchors.fill: parent
        }
    }

    Item {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom

        Editor {
            id: vplEditor
            anchors.fill: parent

            // improve using: https://appbus.wordpress.com/2016/05/20/one-page-sample-app/
            FloatingActionButton {
                id:runButton
                property int distToBorders: isMini ? 16 : 24
                anchors.right: parent.right
                anchors.rightMargin: (isMini ? 64 : 96) + distToBorders
                anchors.bottom: parent.bottom
                anchors.bottomMargin: distToBorders
                isMini: window.width <= 460
                imageSource: !thymio.playing ? "icons/ic_play_arrow_white_24px.svg" : "icons/ic_stop_white_24px.svg"
                visible: !vplEditor.blockEditorVisible
                onClicked: thymio.playing = !thymio.playing
                enabled: (vplEditor.compiler.output.error === undefined || !vplEditor.compiler.output.error) && thymio.node !== undefined
                opacity: enabled ? 1.0 : 0.38
            }
        }
        ThymioSelectionPane {
            id:thymioselectionpane
            anchors.fill: parent
            visible : true
            aseba: aseba
            onSelectedChanged: {
                visible = false
                thymio.node = aseba.createNode(thymioselectionpane.selected)
            }
            onItemSelected: {
                visible = false
            }
        }
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
            whiteIcon: "icons/ic_open_white_24px.svg";
            blackIcon: "icons/ic_open_black_24px.svg";
        }
        function load() {
            saveProgramDialog.isSave = false;
            saveProgramDialog.visible = true;
        }

        ListElement {
            title: QT_TR_NOOP("save program");
            callback: "save";
            whiteIcon: "icons/ic_save_white_24px.svg";
            blackIcon: "icons/ic_save_black_24px.svg";
        }
        function save() {
            saveProgramDialog.isSave = true;
            saveProgramDialog.visible = true;
        }

        ListElement {
            title: QT_TR_NOOP("new program");
            callback: "newProgram";
            whiteIcon: "icons/ic_new_white_24px.svg";
            blackIcon: "icons/ic_new_black_24px.svg";
        }
        function newProgram() {
            vplEditor.clearProgram();
            saveProgramDialog.programName = "";
        }

        ListElement {
            title: QT_TR_NOOP("switch color theme");
            callback: "switchColorTheme";
            whiteIcon: "icons/ic_invert_colors_white_24px.svg";
            blackIcon: "icons/ic_invert_colors_black_24px.svg";
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
            whiteIcon: "icons/ic_info_white_24px.svg";
            blackIcon: "icons/ic_info_black_24px.svg";
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

    /*DashelTargetDialog {
        id: dashelTargetSelector
        aseba: aseba
        visible: false
    }*/

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
            Label {
                textFormat: Text.RichText;
                text:
                    linkRichTextStyle +
                    qsTr("<p><a href=\"http://stephane.magnenat.net\">Stéphane Magnenat</a>, <a href=\"http://sampla.ch\">Martin Voelkle</a>,<br/><a href=\"http://mariamari-a.com\">Maria Beltran</a> and contributors.</p>") +
                    qsTr("<p>© 2015–2017 EPFL, ETH Zürich and Mobsya</p>") +
                    qsTr("<p>This project is open source under <a href=\"https://github.com/aseba-community/thymio-vpl2/blob/master/LICENSE.txt\">LGPL</a>.<br/>") +
                    qsTr("See file <a href=\"https://github.com/aseba-community/thymio-vpl2/blob/master/AUTHORS.md\">AUTHORS.md</a> in the <a href=\"https://github.com/aseba-community/thymio-vpl2\">source code</a><br/>") +
                    qsTr("for a detailed list of contributions.")
                font.pixelSize: 14
                font.weight: Font.Normal
                lineHeight: 20
                lineHeightMode: Text.FixedHeight
                onLinkActivated: Qt.openUrlExternally(link)
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                Rectangle {
                    width: 100
                    height: 50
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                    HDPIImage {
                        width: 80
                        height: width * 0.5
                        fillMode: Image.PreserveAspectFit
                        source: "images/logoEPFL.svg"
                        anchors.centerIn: parent
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: { Qt.openUrlExternally("http://mobots.epfl.ch"); }
                    }
                }
                Rectangle {
                    width: 100
                    height: 50
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                    HDPIImage {
                        width: 80
                        height: width * 0.48 // 0.38753
                        fillMode: Image.PreserveAspectFit
                        source: "images/logoETHGTC.svg"
                        anchors.centerIn: parent
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: { Qt.openUrlExternally("http://www.gtc.inf.ethz.ch"); }
                    }
                }
                Rectangle {
                    width: 100
                    height: 50
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                    HDPIImage {
                        width: 80
                        height: width * 0.27348
                        fillMode: Image.PreserveAspectFit
                        source: "images/logoMobsya.svg"
                        anchors.centerIn: parent
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: { Qt.openUrlExternally("http://www.mobsya.org"); }
                    }
                }
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
            TextEdit {
                id: aeslSourceText
                text: prettyPrintGeneratedAesl(vplEditor.compiler.output.script)
                color: Material.primaryTextColor
                font.family: "Monospace"
                readOnly: true
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
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        parent.selectAll()
                        parent.copy()
                        parent.deselect()
                    }
                }
            }
            contentWidth: aeslSourceText.paintedWidth
            contentHeight: aeslSourceText.paintedHeight
            ScrollBar.vertical: ScrollBar { }
            ScrollBar.horizontal: ScrollBar { }
        }
    }

    AsebaClient {
        id: aseba
        /*onConnectionError: {
            timer.start();
        }*/
    }


    /*Aseba {
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
    }*/

    Thymio {
        id: thymio
        property bool playingOld : false
        property bool playing: false
        onNodeChanged: playing = false
        onPlayingChanged: {
            if (playing && !playingOld) {
                start();
            } else if (!playing && playingOld) {
                stop();
            }
            playingOld = playing;
        }
        function start() {
            vplEditor.saveProgram(autosaveName);
            vplEditor.compiler.execReset(true);
            var output = vplEditor.compiler.output;
            program = {
                events: output.events,
                source: output.script,
            };
        }
        function stop() {
            vplEditor.compiler.execReset(false);
            program = {
                events: {},
                source: "",
            };
        }
        onErrorChanged: vplEditor.compiler.output.error = error;

        onReadyChanged: {
           runButton.enabled = (vplEditor.compiler.output.error === undefined || !vplEditor.compiler.output.error) && thymio.node !== undefined
        }
    }

    Component.onCompleted: {
        vplEditor.loadProgram(autosaveName);
    }
}

