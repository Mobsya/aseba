import QtQuick 2.5
import QtQuick.Window 2.2

Window {
	visible: true
	width: 1280
	height: 720
	color: "white"

	// to get screen coordinates
	Item {
		id: screen
		anchors.fill: parent
	}

	// container for main view
	PinchArea {
		id: pinchArea

		anchors.fill: parent

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
					scene.x += (scene.x - screen.width/2) * deltaScale / scene.scale;
					scene.y += (scene.y - screen.height/2) * deltaScale / scene.scale;
					scene.scale += deltaScale;
				}
			}
		}

		Item {
			id: scene

			property int highestZ: 2

			property real vx: 0 // in px per millisecond
			property real vy: 0 // in px per millisecond

			// we use a timer to have some smooth effect
			// TODO: fixme
			Timer {
				id: accelerationTimer
				interval: 17
				repeat: true
				onTriggered: {
					x += (vx * interval) * 0.001;
					y += (vy * interval) * 0.001;
					vx *= 0.85;
					vy *= 0.85;
					if (Math.abs(vx) < 1 && Math.abs(vy) < 1)
					{
						running = false;
						vx = 0;
						vy = 0;
					}
					console.log(vx);
					console.log(vy);
				}
			}

			// container for all links
			Item {
				id: linkContainer
			}

			// container for all blocks
			Item {
				id: blockContainer

				// timer to desinterlace objects
				Timer {
					interval: 17
					repeat: true
					running: true

					function sign(v) {
						if (v > 0)
							return 1;
						else if (v < 0)
							return -1;
						else
							return 0;
					}

					onTriggered: {
						var i, j;
						// move all blocks too close
						for (i = 0; i < blockContainer.children.length; ++i) {
							for (j = 0; j < i; ++j) {
								var dx = blockContainer.children[i].x - blockContainer.children[j].x;
								var dy = blockContainer.children[i].y - blockContainer.children[j].y;
								var dist = Math.sqrt(dx*dx + dy*dy);
								if (dist < 310) {
									var normDist = dist;
									var factor = 100 / (normDist+1);
									blockContainer.children[i].x += sign(dx) * factor;
									blockContainer.children[j].x -= sign(dx) * factor;
									blockContainer.children[i].y += sign(dy) * factor;
									blockContainer.children[j].y -= sign(dy) * factor;
								}
							}
						}
					}
				}
			}

			property double prevTime: 0

			Drag.onDragStarted: {
				prevTime = new Date().valueOf();
				console.log("drag started");
			}
		}
	}

	// add block
	Rectangle {
		id: addBlock

		width: 128
		height: 128
		radius: 64
		color: "gray"
		anchors.left: parent.left
		anchors.leftMargin: 20
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 20

		MouseArea {
			anchors.fill: parent
			onClicked: {
				if (editor.visible)
					return;
				var center = blockContainer.mapToItem(screen, screen.width/2, screen.height/2);
				var blockComponent = Qt.createComponent("Block.qml");
				var block = blockComponent.createObject(blockContainer, {
					x: center.x - 128 + Math.random(),
					y: center.y - 128 + Math.random()
				});
				editor.editedBlock = block;
				editor.visible = true;
			}
		}
	}

	// delete block
	DropArea {
		id: delBlock
		// FIXME: how to get this area receive drop for blocks, knowing they are rescaled?

		width: 128
		height: 128

		anchors.right: parent.right
		anchors.rightMargin: 20
		anchors.bottom: parent.bottom
		anchors.bottomMargin: 20

		Rectangle {
			anchors.fill: parent
			radius: 64
			color: "gray"
		}

		onEntered: {
			console.log("entered");
		}

		onExited: {
			console.log("exited");
		}
	}

	// center view
	Rectangle {
		id: centerView

		width: 96
		height: 96
		radius: 48
		color: "gray"
		anchors.right: parent.right
		anchors.rightMargin: 20+16
		anchors.top: parent.top
		anchors.topMargin: 20+16

		MouseArea {
			anchors.fill: parent
			onClicked: {
				scene.x = screen.width/2 - (blockContainer.childrenRect.x + blockContainer.childrenRect.width/2) * scene.scale;
				scene.y = screen.height/2 - (blockContainer.childrenRect.y + blockContainer.childrenRect.height/2) * scene.scale;
			}
		}
	}

	Editor {
		id: editor
	}
}

