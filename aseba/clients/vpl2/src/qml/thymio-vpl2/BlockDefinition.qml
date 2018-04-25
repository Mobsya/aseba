import QtQuick 2.7

Item {
	// This must be a JSON object to allow proper serialization
	property var defaultParams
	property bool advanced: false
	// editor must have a member params that will be initialized either with defaultParams
	// or with the previously-set params, and a method getParams() that should return
	// the edited parameters. Values must be JSON objects
	property Component editor
	// miniature must have a member params like editor, but does not need getParams()
	property Component miniature: editor
	property string type
	property string category
	function compile(params) {
		return {};
	}
}
