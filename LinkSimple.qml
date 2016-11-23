import QtQuick 2.0

Link {
	visible: false
	property bool ast: sourceBlock !== destBlock && sourceBlock.definition !== null && destBlock.definition !== null
	onAstChanged: {
		if (ast) {
			scene.ast.links.push(this);
		} else {
			var index = scene.ast.links.indexOf(this);
			scene.ast.links.splice(index, 1);
		}
	}
}
