import QtQuick 2.7
import QtQuick.Layouts 1.3

RowLayout {
	id: blocks

	property string typeRestriction

	property var ast: []

	function append(definition, params) {
		var properties = {
			index: children.length,
			definition: definition,
			params: params,
		};
		component.createObject(blocks, properties);
	}

	Component {
		id: component
		Block {
			id: block

			property int index
			typeRestriction: blocks.typeRestriction

			Component.onCompleted: {
				ast.splice(index, 0, this);
				for (var i = index + 1; i < ast.length; ++i) {
					var block = ast[i];
					block.index = i;
				}
				astChanged();
			}

			Component.onDestruction: {
				ast.splice(index, 1);
				for (var i = index; i < ast.length; ++i) {
					var block = ast[i];
					block.index = i;
				}
				astChanged();
			}

			onParamsChanged: {
				astChanged();
			}
		}
	}
}
