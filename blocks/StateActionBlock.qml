import QtQuick 2.7
import QtMultimedia 5.8
import ".."

BlockDefinition {
	type: "action"
	category: "state"

	defaultParams: { "image": "images/nop.svg" }

	editor: Component {
		Item {
			width: 256
			height: 256
			property var params: defaultParams

			VideoOutput {
				id: output
				// FIXME: resolve scope
				source: vplEditor.camera
				anchors.fill: parent
				fillMode: Image.PreserveAspectCrop
				focus : visible // to receive focus and capture key events when visible

				MouseArea {
					anchors.fill: parent;
					onClicked: shader.grabToImage(function(result) { capturedImage.source = result.url; })
				}
			}
			ShaderEffect {
				id: shader
				property variant source: ShaderEffectSource { sourceItem: output; hideSource: true }
				property color transparent: "transparent"
				anchors.fill: output

				fragmentShader: "
					varying highp vec2 qt_TexCoord0;
					uniform sampler2D source;
					uniform vec4 transparent;
					void main(void)
					{
						if (length(qt_TexCoord0 - vec2(0.5, 0.5)) < 0.5)
							gl_FragColor = texture2D(source, qt_TexCoord0);
						else
							gl_FragColor = transparent;
					}
				"
			}

			Image {
				id: capturedImage
				x: -192/2
				y: -192/2
				source: params.image
				scale: 0.25
			}

			function getParams() {
				return { "image": capturedImage.source };
			}
		}
	}

	miniature: Component {
		Item {
			width: 256
			height: 256
			property var params: defaultParams

			Image {
				anchors.centerIn: parent
				scale: 0.8
				source: params.image
			}
		}
	}

	function compile(params) {
		return {
			action: ""
		};
	}
}
