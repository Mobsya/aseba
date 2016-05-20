import QtQuick 2.0

Component {
	HDPIImage {
		source: "images/linkEndArrow.svg"
		width: 32 // working around Qt bug with SVG and HiDPI
		height: 32 // working around Qt bug with SVG and HiDPI

		property string linkName: "arrow"

		property Item sourceBlock: Null
		property Item destBlock: Null
	}
}
