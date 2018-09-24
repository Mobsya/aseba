import QtQuick 2.0

ListModel {
    id: appsModel


    function launch_blockly(device) {
        const baseurl = Utils.webapp_base_url("blockly");
        if(!baseurl) {
            return false;
        }
        const url = "%1/thymio_blockly.en.html#device=%2&ws=%3"
            .arg(baseurl).arg(device.id).arg(device.websocketEndpoint)
        return Qt.openUrlExternally(url)
    }

    property var launch_functions : {
        "blockly" : launch_blockly
    }


    function launch_function(app) {
        const appId = app.appId
        console.log(appId)
        return launch_functions[appId]
    }

//    ListElement {
//        name: "VPL"
//        animatedIcon:"qrc:/apps/vpl/vpl-animated-icon.webp"
//    }
//    ListElement {
//        name: "Scatch"
//        animatedIcon:"qrc:/apps/scratch/scratch-animated-icon.webp"
//    }

    ListElement {
        appId: "blockly"
        name: "Blockly"
        animatedIcon: "qrc:/apps/blockly/blockly-animated-icon.webp"
        icon: "qrc:/apps/blockly/blockly-icon.svg"
        descriptionImage: "qrc:/apps/blockly/description.jpeg"
    }

//    ListElement {
//        name: "Studio"
//        animatedIcon:"qrc:/apps/studio/studio-animated-icon.webp"
//    }
}
