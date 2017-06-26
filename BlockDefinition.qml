import QtQuick 2.7

Item {
	property var defaultParams
	property bool advanced: false
	property Component editor
	property Component miniature: editor
	property string type
	property string category
	function compile(params) {
		return {};
	}
}
