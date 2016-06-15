import QtQuick 2.0
import Aseba 1.0

Item {
	property string target: Qt.platform.os === "android" ? "android:" : Qt.platform.os === "osx" ? "ser:name=Thymio-II" : "ser:device=/dev/ttyACM0"
	property alias nodes: client.nodes
	signal userMessage(int type, var data)

	AsebaClient {
		id: client
		onConnectionError: {
			timer.start();
		}
	}

	function startClient() {
		client.start(target);
	}

	// FIXME: this does not work if the connection is already active
	onTargetChanged: {
		console.log("connecting to new target", target);
		startClient();
	}

	Component.onCompleted: {
		client.userMessage.connect(userMessage);
		startClient();
	}

	Timer {
		id: timer
		interval: 1000
		onTriggered: startClient()
	}
}
