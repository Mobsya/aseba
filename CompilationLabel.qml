import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Label {
	property Editor vplEditor

	text: {
		if (!vplEditor)
			return "!Error no VPL editor defined!";
		else if (vplEditor.compiler.output.error === undefined)
			return qsTr("Compilation success");
		else
			return qsTr("Compilation error: %0").arg(vplEditor.compiler.output.error);
	}

	horizontalAlignment: Text.AlignLeft
	verticalAlignment: Text.AlignVCenter
	font.pixelSize: 20
	font.weight: Font.Medium
	elide: Label.ElideRight
	wrapMode: Text.Wrap
	Layout.fillWidth: true
}
