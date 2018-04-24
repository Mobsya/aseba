import QtQuick 2.7

HDPIImage {
	id: centerImageId
	property string typeRestriction
	property BlockDefinition definition
	source: {
		if (definition === null) {
			switch(typeRestriction) {
			case "event": return "images/eventPlaceholder.svg";
			case "action": return "images/actionPlaceholder.svg";
			}
		} else {
			switch(definition.type) {
			case "event": return "images/eventCenter.svg";
			case "action": return "images/actionCenter.svg";
			}
		}
		return "";
	}
	scale: 0.72
	width: 256 // working around Qt bug with SVG and HiDPI
	height: 256 // working around Qt bug with SVG and HiDPI
}
