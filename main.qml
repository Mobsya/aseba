import QtQuick 2.5
import QtQuick.Window 2.2

Window {
	visible: true
	width: 1280
	height: 720
	color: "white"

	PinchArea {
		anchors.fill: parent

		onPinchUpdated: {
			// resize content
			var deltaScale = pinch.scale - pinch.previousScale

			// adjust content pos due to scale
			scene.contentX -= (-scene.contentX - pinch.center.x) * deltaScale / scene.contentItem.scale;
			scene.contentY -= (-scene.contentY - pinch.center.y) * deltaScale / scene.contentItem.scale;
			scene.contentItem.scale += deltaScale;

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

			EventBlock { x: 0; y: 0 }
			ActionBlock { x: 650; y: 350 }
			EventBlock { x: 500; y: 50 }
			ActionBlock { x: 150; y: 300 }

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
}

