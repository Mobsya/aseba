import QtQuick 2.0

ListModel {
    id: appsModel


    function launch_blockly(device) {
        const baseurl = Utils.webapp_base_url("blockly");
        if(!baseurl) {
            return false;
        }
        const url = "%1/thymio_blockly.en.html#device=%2&ws=%3"
            .arg(baseurl).arg(device.id).arg(device.websocketEndpoint())
        return Utils.openUrl(url)
    }

    function launch_scratch(device) {
        const url = "http://0.0.0.0:8601/#device=%2&ws=%3".arg(device.id).arg(device.websocketEndpoint())
        return Utils.openUrl(url)
    }

    function launch_studio(device) {
        var program = Utils.search_program("asebastudio")
        if(!program)
            return false;
        return Utils.launch_process(program, ["--uuid", device.id])
    }

    function launch_vplClassic(device) {
        var program = Utils.search_program("thymiovplclassic")
        if(!program)
            return false;
        return Utils.launch_process(program, ["--uuid", device.id])
    }

    property var launch_functions : {
        "blockly" : launch_blockly,
        "scratch" : launch_scratch,
        "vplClassic" : launch_vplClassic,
        "studio" : launch_studio
    }


    function launch_function(app) {
        const appId = app.appId
        console.log(appId)
        return launch_functions[appId]
    }

    ListElement {
        appId:"vplClassic"
        name: "VPL"
        animatedIcon:"qrc:/apps/vpl/vpl-animated-icon.webp"
        icon: "qrc:/apps/vpl/launcher-icon-vpl.svg"
    }
    ListElement {
        appId: "scratch"
        name: "Scratch"
        animatedIcon:"qrc:/apps/scratch/scratch-animated-icon.webp"
        icon: "qrc:/apps/scratch/launcher-icon-scratch.svg"
    }

    ListElement {
        appId: "blockly"
        name: "Blockly"
        animatedIcon: "qrc:/apps/blockly/blockly-animated-icon.webp"
        icon: "qrc:/apps/blockly/blockly-icon.svg"
        descriptionImage: "qrc:/apps/blockly/description.jpeg"
    }

     ListElement {
         appId:"studio"
         name: "Studio"
         animatedIcon:"qrc:/apps/studio/studio-animated-icon.webp"
         descriptionImage: "qrc:/apps/studio/description.jpg"
         icon: "qrc:/apps/studio/launcher-icon-studio.svg"
     }
}
