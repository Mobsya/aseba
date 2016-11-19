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

	property alias compiler: compiler
	Compiler {
		id: compiler
		ast: scene.ast
	}

	property list<BlockDefinition> eventDefinitions: [
		ButtonsEventBlock {},
		ProxEventBlock {},
		ProxGroundEventBlock {},
		TapEventBlock {},
		ClapEventBlock {},
		TimerEventBlock {}
	]

	property list<BlockDefinition> actionDefinitions: [
		MotorActionBlock {},
		PaletteTopColorActionBlock {},
		PaletteBottomColorActionBlock {},
		TopColorActionBlock {},
		BottomColorActionBlock {}
	]

	function programsDB() {
		return LocalStorage.openDatabaseSync("Programs", "1.0", "Locally saved programs", 100000);
	}

	function saveProgram(name) {
		programsDB().transaction(
			function(tx) {
				// Create the database if it doesn't already exist
				tx.executeSql('CREATE TABLE IF NOT EXISTS Programs(name TEXT PRIMARY KEY, code TEXT)');
				// Add (another) program
				tx.executeSql('INSERT OR REPLACE INTO Programs VALUES(?, ?)', [ name, JSON.stringify(serialize()) ]);
			}
		)
	}

	function listPrograms() {
		var programs;
		try {
			programsDB().readTransaction(
				function(tx) {
					// List existing programs
					programs = tx.executeSql('SELECT * FROM Programs ORDER BY name').rows;
				}
			)
		}
		catch (err) {
			console.log("list program made an error: ", err);
			programs = [];
		}
		return programs;
	}

	function loadProgram(name) {
		var rows;
		try {
			programsDB().readTransaction(
				function(tx) {
					// List existing programs
					rows = tx.executeSql('SELECT code FROM Programs WHERE name = ?', name).rows;
				}
			)
		}
		catch (err) {
			console.log("load program made an error: ", err);
			return;
		}
		if (rows.length === 0)
			return;
		var row = rows[0];
		scene.deserialize(JSON.parse(row.code));

		// then reset view
		mainContainer.fitToView();
	}

	function clearProgram() {
		scene.clear();
		mainContainer.fitToView();
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
		onWidthChanged: scene.x = width/2 - (scene.viewRect.x + scene.viewRect.width/2) * scene.scale;
		onHeightChanged: scene.y = height/2 - (scene.viewRect.y + scene.viewRect.height/2) * scene.scale;

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

				EditorAdvanced {
					id: scene
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
