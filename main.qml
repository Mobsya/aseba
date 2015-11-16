import QtQuick 2.5
import QtQuick.Window 2.2

Window {
	visible: true
	width: 1280
	height: 720
	color: "white"

	Item {
		id: screen
		anchors.fill: parent
	}

	PinchArea {
		id: pinchArea

		anchors.fill: parent

		onPinchUpdated: {
			// resize content
			var deltaScale = pinch.scale - pinch.previousScale

			// adjust content pos due to scale
			scene.contentX -= (-scene.contentX - pinch.center.x) * deltaScale / scene.contentItem.scale;
			scene.contentY -= (-scene.contentY - pinch.center.y) * deltaScale / scene.contentItem.scale;
			scene.contentItem.scale += deltaScale;
			scene.contentItem.scale = Math.max(scene.contentItem.scale, 1e-2);

			// adjust content pos due to drag
			scene.contentX -= pinch.center.x - pinch.previousCenter.x;
			scene.contentY -= pinch.center.y - pinch.previousCenter.y;
		}

		onPinchFinished: {
			// TODO: check if childrenrect is scaled or not, once non 0
			// ensure that at least something is visible
			if (scene.contentItem.x + scene.contentItem.childrenRect.x + scene.contentItem.childrenRect.width * scene.contentItem.scale < 0) {
				scene.contentItem.x = width - scene.contentItem.childrenRect.x - scene.contentItem.childrenRect.width * scene.contentItem.scale;
			}
			if (scene.contentItem.x + scene.contentItem.childrenRect.x > width) {
				scene.contentItem.x = -scene.contentItem.childrenRect.x;
			}
			if (scene.contentItem.y + scene.contentItem.childrenRect.y + scene.contentItem.childrenRect.height * scene.contentItem.scale < 0) {
				scene.contentItem.y = height - scene.contentItem.childrenRect.y - scene.contentItem.childrenRect.height * scene.contentItem.scale;
			}
			if (scene.contentItem.y + scene.contentItem.childrenRect.y > height) {
				scene.contentItem.y = -scene.contentItem.childrenRect.y;
			}
			console.log(scene.contentItem.x)
			console.log(scene.contentItem.y)
		}

		MouseArea {
			anchors.fill: parent
			drag.target: scene
			scrollGestureEnabled: false
		}

		Flickable {
			id: scene

			//anchors.fill: parent

			contentItem.transformOrigin: Item.TopLeft

			property int highestZ: 2

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

			Component.onCompleted: {
				setContentSize();
			}

			function setContentSize() {
				// FIXME: there is still a bug with zooming
				//console.log(scene.contentWidth)
				//console.log(scene.contentItem.x)
				//scene.contentWidth = scene.contentItem.childrenRect.width * scene.contentItem.scale;
				//scene.contentHeight = scene.contentItem.childrenRect.height * scene.contentItem.scale;
				//console.log(scene.contentWidth)
				//console.log(scene.contentItem.x)
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

		property bool createEvent: false

		MouseArea {
			anchors.fill: parent
			onClicked: {
				var center = blockContainer.mapToItem(screen, screen.width/2, screen.height/2);
				if (addBlock.createEvent) {
					var eventBlockComponent = Qt.createComponent("EventBlock.qml");
					eventBlockComponent.createObject(blockContainer, {
						x: center.x - 128 + Math.random(),
						y: center.y - 128 + Math.random()
					});
					addBlock.createEvent = false;
				} else {
					var actionBlockComponent = Qt.createComponent("ActionBlock.qml");
					actionBlockComponent.createObject(blockContainer, {
						x: center.x - 128 + Math.random(),
						y: center.y - 128 + Math.random()
					});
					addBlock.createEvent = true;
				}
			}
		}
	}
}

