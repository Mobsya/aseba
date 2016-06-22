import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Label {
	property Editor vplEditor

	text: {
		if (!vplEditor)
			return "";
		else  if (vplEditor.minimized)
			return qsTr("AR Running");
		else if (vplEditor.blockEditorVisible)
			return qsTr("Choose a block and set its parameters");
		else if (vplEditor.compiler.error === "")
			return qsTr("Compilation success");
		else
			return qsTr("Compilation error: ") + vplEditor.compiler.error;
	}

	horizontalAlignment: Text.AlignHCenter
	font.pixelSize: 20
	elide: Label.ElideRight
	verticalAlignment: Text.AlignVCenter
	Layout.fillWidth: true
}
