import QtQuick 2.7
import QtQuick.Window 2.2

Image {
	// working around Qt bug with SVG and HiDPI
	sourceSize.width: width * Screen.devicePixelRatio
	sourceSize.height: height * Screen.devicePixelRatio
}
