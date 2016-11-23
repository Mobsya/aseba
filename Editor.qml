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
	property alias mainContainerScale: mainContainer.scale
	property alias scene: sceneLoader.item

	property alias compiler: compiler
	Compiler {
		id: compiler
		ast: scene.ast
	}

	property list<BlockDefinition> eventDefinitions
	property list<BlockDefinition> actionDefinitions
	property var definitions: {
		var definitions = {};

		function loadBlockDefinition(name) {
			var url = "blocks/" + name + ".qml";
			var component = Qt.createComponent(url);
			var properties = {
				objectName: name,
			};
			var object = component.createObject(vplEditor, properties);
			definitions[name] = object;
			return object;
		}

		eventDefinitions = [
			"ButtonsEventBlock",
			"ProxEventBlock",
			"ProxGroundEventBlock",
			"TapEventBlock",
			"ClapEventBlock",
			"TimerEventBlock",
		].map(loadBlockDefinition);

		actionDefinitions = [
			"MotorActionBlock",
			"PaletteTopColorActionBlock",
			"PaletteBottomColorActionBlock",
			"TopColorActionBlock",
			"BottomColorActionBlock",
		].map(loadBlockDefinition);

		return definitions;
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
		mainContainer.fitToView();
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

		blocks: eventDefinitions
		backImage: "images/eventCenter.svg"

		darkThemeColor: "#301446"
		lightThemeColor: "#ffead9"

		anchors.left: parent.left
		anchors.leftMargin: vplEditor.minimized ? -width : 0

		Behavior on anchors.leftMargin { PropertyAnimation {} }
	}

	BlocksPane {
		id: actionPane

		blocks: actionDefinitions
		backImage: "images/actionCenter.svg"

		darkThemeColor: "#301446"
		lightThemeColor: "#daeaf2"

		anchors.right: parent.right
		anchors.rightMargin: vplEditor.minimized ? -width : 0

		Behavior on anchors.rightMargin { PropertyAnimation {} }
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
		color: vplEditor.minimized ? "#80200032" : "#ff44285a"
		scale: vplEditor.minimized ? 0.5 : 1.0
		transformOrigin: Item.BottomRight

		RadialGradient {
			anchors.fill: parent
			visible: Material.theme === Material.Light
			gradient: Gradient {
				GradientStop { position: 0.0; color: "white" }
				GradientStop { position: 0.5; color: "#eaeced" }
			}
		}

		Behavior on opacity { PropertyAnimation {} }
		Behavior on color { PropertyAnimation {} }
		Behavior on scale { PropertyAnimation {} }
		Behavior on anchors.rightMargin { PropertyAnimation {} }

		Component.onCompleted: fitToView()
		// fit all contents to the view
		function fitToView() {
			var rect = scene.viewRect;

			var scale;
			if (rect.width === 0 || rect.height === 0) {
				scale = 0.5;
			} else {
				scale = Math.min(0.5, width * 0.7 / rect.width, height * 0.7 / rect.height);
			}

			scene.scale = scale;
			scene.x = width/2 - (rect.x + rect.width/2) * scale;
			scene.y = height/2 - (rect.y + rect.height/2) * scale;
		}

		// keep the center of the scene at the center of the mainContainer
		onWidthChanged: if (scene !== null) { scene.x = width/2 - (scene.viewRect.x + scene.viewRect.width/2) * scene.scale; }
		onHeightChanged: if (scene !== null) { scene.y = height/2 - (scene.viewRect.y + scene.viewRect.height/2) * scene.scale; }

		Image {
			anchors.fill: parent
			source: "images/grid.png"
			fillMode: Image.Tile
			opacity: vplEditor.minimized ? 0 : 1

			Behavior on opacity { PropertyAnimation {} }
		}

		// container for main view
		PinchArea {
			id: pinchArea

			anchors.fill: parent
			clip: true

			property double prevTime: 0

			onPinchStarted: {
				prevTime = new Date().valueOf();
			}

			onPinchUpdated: {
				var deltaScale = pinch.scale - pinch.previousScale

				// adjust content pos due to scale
				if (scene.scale + deltaScale > 1e-1) {
					scene.x += (scene.x - pinch.center.x) * deltaScale / scene.scale;
					scene.y += (scene.y - pinch.center.y) * deltaScale / scene.scale;
					scene.scale += deltaScale;
				}

				// adjust content pos due to drag
				var now = new Date().valueOf();
				var dt = now - prevTime;
				var dx = pinch.center.x - pinch.previousCenter.x;
				var dy = pinch.center.y - pinch.previousCenter.y;
				scene.x += dx;
				scene.y += dy;
				//scene.vx = scene.vx * 0.6 + dx * 0.4 * dt;
				//scene.vy = scene.vy * 0.6 + dy * 0.4 * dt;
				prevTime = now;
			}

			onPinchFinished: {
				//accelerationTimer.running = true;
			}

			MouseArea {
				anchors.fill: parent
				drag.target: scene
				scrollGestureEnabled: false

				onWheel: {
					var deltaScale = scene.scale * wheel.angleDelta.y / 1200.;

					// adjust content pos due to scale
					if (scene.scale + deltaScale > 1e-1) {
						scene.x += (scene.x - mainContainer.width/2) * deltaScale / scene.scale;
						scene.y += (scene.y - mainContainer.height/2) * deltaScale / scene.scale;
						scene.scale += deltaScale;
					}
				}
			}

			DropArea {
				id: mainDropArea
				anchors.fill: parent

				onDropped: {
					if (drop.source === blockDragPreview) {
						var pos = mainDropArea.mapToItem(scene, drop.x, drop.y);
						createBlock(pos.x, pos.y, blockDragPreview.definition);
					} else if (drop.hasText) {
						try {
							loadCode(drop.text);
						} catch (e) {
							drop.accepted = false;
						}
					} else {
						drop.accepted = false;
					}
				}

				Loader {
					id: sceneLoader

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
						mainContainer.fitToView();
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

		// center view
		Item {
			width: 48
			height: 48
			anchors.right: parent.right
			anchors.rightMargin: 4
			anchors.bottom: parent.bottom
			anchors.bottomMargin: 4

			HDPIImage {
				source: Material.theme === Material.Light ? "icons/ic_gps_fixed_black_24px.svg" : "icons/ic_gps_fixed_white_24px.svg"
				width: 24 // working around Qt bug with SVG and HiDPI
				height: 24 // working around Qt bug with SVG and HiDPI
				anchors.centerIn: parent
				visible: !minimized
			}

			MouseArea {
				anchors.fill: parent
				onClicked: mainContainer.fitToView();
			}
		}

		// block editor
		BlockEditor {
			id: blockEditor
		}
	}
}
