import QtQuick 2.5
import QtQuick.Window 2.2
import QtMultimedia 5.5
import QtQuick.Controls 2.0
import QtQuick.Controls.Material 2.0
import QtGraphicalEffects 1.0
import QtQuick.LocalStorage 2.0
import "blocks"

Item {
	id: vplEditor

	// whether VPL editor is minimized on top of background image
	property bool minimized: false
	property alias blockEditorVisible: blockEditor.visible
	property BlockEditor blockEditor: blockEditor
	property alias mainContainerScale: mainContainer.scale
	property alias scene: sceneLoader.item

	property alias compiler: compiler
	Compiler {
		id: compiler
		ast: scene.ast
	}

	property var definitions: [
		"ButtonsEventBlock",
		"ProxEventBlock",
		"ProxGroundEventBlock",
		"TapEventBlock",
		"ClapEventBlock",
		"TimerEventBlock",
		"MotorActionBlock",
		//"MotorSimpleActionBlock",
		"PaletteTopColorActionBlock",
		"PaletteBottomColorActionBlock",
		"TopColorActionBlock",
		"BottomColorActionBlock",
	].map(function(name) {
		var url = "blocks/" + name + ".qml";
		var component = Qt.createComponent(url);
		if (component.status === Component.Error) {
			throw component.errorString();
		}

		var properties = {
			objectName: name,
		};
		var object = component.createObject(vplEditor, properties);
		return object;
	});
	property var definitionsByName: {
		var definitionsByName = {};
		definitions.forEach(function(definition) {
			definitionsByName[definition.objectName] = definition;
		});
		return definitionsByName;
	}

	function programsDB() {
		var db = LocalStorage.openDatabaseSync("Programs", "", "Locally saved programs", 100000);

		function updateDB(loadAndClearDB) {
			db.changeVersion(db.version, "2", function(tx) {
				var programs = loadAndClearDB(tx);
				tx.executeSql("create table programs (name text not null primary key, code text not null)");
				for (var i = 0; i < programs.length; ++i) {
					var program = programs[i];
					var args = [
						program.name,
						program.code,
					];
					tx.executeSql("insert into programs (name, code) values (?, ?, ?)", args);
				}
			});
		}

		switch(db.version) {
		case "":
			updateDB(function(tx) {
				return [];
			});
			break;
		case "1.0":
			updateDB(function(tx) {
				if (tx.executeSql("select 1 from sqlite_master where type = 'table' and name = 'programs'").rows.length === 0) {
					// the programs table was not created
					return [];
				}

				var programsOld = tx.executeSql("select name, code from programs").rows;
				var programsNew = [];
				for (var i = 0; i < programsOld.length; ++i) {
					var programOld = programsOld[i];
					var nameOld = programOld.name;
					var codeOld = programOld.code;

					var nameNew = nameOld;
					var codeNew = "{\"mode\":\"advanced\",\"scene\":" + codeOld + "}";
					var programNew = {
						name: nameNew,
						code: codeNew,
					};

					programsNew.push(programNew);
				}

				tx.executeSql("drop table programs");
				return programsNew;
			});
			break;
		}
		return db;
	}

	function saveProgram(name) {
		// Add (another) program
		var code = {
			mode: sceneLoader.mode,
			scene: scene.serialize(),
		};
		code = JSON.stringify(code);
		console.log(code);
		programsDB().transaction(function(tx) {
			var args = [ name, code ];
			tx.executeSql("insert or replace into programs (name, code) values (?, ?)", args);
		});
	}

	function listPrograms() {
		// List existing programs
		var rows;
		programsDB().readTransaction(function(tx) {
			rows = tx.executeSql("select name from programs order by name").rows;
		});
		return rows;
	}

	function loadProgram(name) {
		// Load existing program
		var rows;
		programsDB().readTransaction(function(tx) {
			rows = tx.executeSql("select code from programs where name = ?", name).rows;
		});
		if (rows.length === 0)
			return;
		var row = rows[0];
		loadCode(row.code);
	}
	function loadCode(string) {
		var code = JSON.parse(string);
		sceneLoader.mode = code.mode;
		sceneLoader.scene = code.scene;
	}

	function clearProgram() {
		scene.clear();
	}

	function switchMode() {
		switch (sceneLoader.mode) {
		case "simple":
			var sceneAdvanced = serializeAdvanced();
			sceneLoader.mode = "advanced";
			sceneLoader.scene = sceneAdvanced;
			break;
		case "advanced":
			if (scene.blocks.length === 0) {
				sceneLoader.mode = "simple";
			}
			break;
		}
	}

	function serializeAdvanced() {
		var blocksIn = scene.ast.blocks;
		var blocksOut = [];
		for (var i = 0; i < blocksIn.length; ++i) {
			var blockIn = blocksIn[i];
			var blockOut = blockIn.serialize(true, i);
			blocksOut.push(blockOut);
		}

		var linksIn = scene.ast.links;
		var linksOut = [];
		for (var i = 0; i < linksIn.length; ++i) {
			var linkIn = linksIn[i];
			var linkOut = linkIn.serialize();
			linksOut.push(linkOut);
		}

		return {
			blocks: blocksOut,
			links: linksOut,
		};
	}

	BlocksPane {
		id: eventPane
		property bool isHidden: vplEditor.minimized || (blockEditor.typeRestriction !== "" && blockEditor.typeRestriction !== "event")

		blocks: definitions.filter(function(definition) {
			return definition.type === "event" && (sceneLoader.mode === "advanced" || !definition.advanced);
		})
		backImage: "images/eventCenter.svg"

		darkThemeColor: "#301446"
		lightThemeColor: Material.background // "#ffead9"

		anchors.left: parent.left
		anchors.leftMargin: isHidden ? -width : 0

		//Behavior on anchors.leftMargin { PropertyAnimation {} }
	}

	BlocksPane {
		id: actionPane
		property bool isHidden: vplEditor.minimized || (blockEditor.typeRestriction !== "" && blockEditor.typeRestriction !== "action")

		blocks: definitions.filter(function(definition) {
			return definition.type === "action" && (sceneLoader.mode === "advanced" || !definition.advanced);
		})
		backImage: "images/actionCenter.svg"

		darkThemeColor: "#301446"
		lightThemeColor: Material.background // "#daeaf2"

		anchors.right: parent.right
		anchors.rightMargin: isHidden ? -width : 0

		//Behavior on anchors.rightMargin { PropertyAnimation {} }
	}

	Rectangle {
		id: mainContainer

		property real foregroundWidth: parent.width - eventPane.width - actionPane.width

		anchors.right: parent.right
		anchors.bottom: parent.bottom
		anchors.rightMargin: vplEditor.minimized ? 0 : actionPane.width

		width: foregroundWidth
		height: parent.height
		//opacity: vplEditor.minimized ? 0.5 : 1.0
		color: Material.theme === Material.Dark ?
				   "#ff44285a" : //vplEditor.minimized ? "#80200032" : "#ff44285a" // different color for AR, to adapt when reused
				   Qt.darker(Material.background, 1.2)
		scale: vplEditor.minimized ? 0.5 : 1.0
		transformOrigin: Item.BottomRight

		// disabled as we look for a flatter theme
		/*RadialGradient {
			anchors.fill: parent
			visible: Material.theme === Material.Light
			gradient: Gradient {
				GradientStop { position: 0.0; color: "white" }
				GradientStop { position: 0.5; color: "#eaeced" }
			}
		}*/

		Behavior on opacity { PropertyAnimation {} }
		//Behavior on color { PropertyAnimation {} }
		Behavior on scale { PropertyAnimation {} }
		Behavior on anchors.rightMargin { PropertyAnimation {} }

		// disabled as we look for a flatter theme
		/*Image {
			anchors.fill: parent
			source: "images/grid.png"
			fillMode: Image.Tile
			opacity: vplEditor.minimized ? 0 : 1

			Behavior on opacity { PropertyAnimation {} }
		}*/

		DropArea {
			id: mainDropArea
			anchors.fill: parent

			onDropped: {
				if (drop.hasText) {
					try {
						loadCode(drop.text);
					} catch (e) {
						drop.accepted = false;
					}
				} else {
					scene.handleSceneDrop(this, drop);
				}
			}

			Loader {
				id: sceneLoader
				anchors.fill: parent

				property string mode: "simple"
				onModeChanged: scene = undefined;

				property var scene: undefined
				onSceneChanged: if (item !== null) loaded()

				source: {
					switch (mode) {
					case "simple": return "EditorSimple.qml";
					case "advanced": return "EditorAdvanced.qml";
					}
				}
				onLoaded: {
					if (scene !== undefined) {
						item.deserialize(scene);
					}
				}
			}
		}

		// preview when adding block
		Item {
			id: blockDragPreview
			property BlockDefinition definition: null
			property var params
			width: 256
			height: 256
			visible: Drag.active
			opacity: 0.8
			scale: scene.scale
			Drag.hotSpot.x: width / 2
			Drag.hotSpot.y: height / 2
			Drag.keys: definition === null ? [] : [definition.type]

			BlockBackground {
				id: centerImageId
				definition: blockDragPreview.definition
				anchors.centerIn: parent
			}

			Loader {
				id: loader
				anchors.centerIn: parent
				sourceComponent: blockDragPreview.definition ? blockDragPreview.definition.miniature : null
				scale: 0.72
				onLoaded: loader.item.params = blockDragPreview.params
			}
		}
	}

	// block editor
	BlockEditor {
		id: blockEditor
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		x: blockEditor.typeRestriction === "event" ?  eventPane.width : 0
		width: parent.width - eventPane.width
		//Behavior on width { PropertyAnimation {} }
		//Behavior on anchors.rightMargin { PropertyAnimation {} }
	}
}
