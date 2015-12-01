import QtQuick 2.0

Component {
	LinkingPath {
		id: blockLink

		property string linkName: "link"

		property Item sourceBlock: Null
		property Item destBlock: Null
	}
}
