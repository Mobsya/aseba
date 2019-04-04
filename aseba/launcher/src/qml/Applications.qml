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
        const baseurl = Utils.webapp_base_url("scratch");
        if(!baseurl) {
            return false;
        }
        const url = "%1/index.html?device=%2&ws=%3"
            .arg(baseurl).arg(device.id).arg(device.websocketEndpoint())
        return Utils.openUrl(url)
    }

    function launch_studio(device) {
        if(Utils.platformIsOsX()) {
            Utils.launchOsXBundle("AsebaStudio", ["--uuid", device.id])
        } else {
            var program = Utils.search_program("asebastudio")
            if(!program)
                return false;
            return Utils.launch_process(program, ["--uuid", device.id])
        }
    }

    function launch_vplClassic(device) {
        if(Utils.platformIsOsX()) {
            Utils.launchOsXBundle("ThymioVPLClassic", ["--uuid", device.id])
        } else {
            var program = Utils.search_program("thymiovplclassic")
            if(!program)
                return false;
            return Utils.launch_process(program, ["--uuid", device.id])
        }
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
        descriptionImage: "qrc:/apps/vpl/description.jpg"
        descriptionTextFile: "qrc:/apps/vpl/desc.en.html"
        supportsGroups: false
        supportsWatchMode: false
    }
    ListElement {
        appId: "scratch"
        name: "Scratch"
        animatedIcon:"qrc:/apps/scratch/scratch-animated-icon.webp"
        icon: "qrc:/apps/scratch/launcher-icon-scratch.svg"
        descriptionImage: "qrc:/apps/scratch/description.jpg"
        descriptionTextFile: "qrc:/apps/scratch/desc.en.html"
        supportsGroups: false
        supportsWatchMode: false
    }

    ListElement {
        appId: "blockly"
        name: "Blockly"
        animatedIcon: "qrc:/apps/blockly/blockly-animated-icon.webp"
        icon: "qrc:/apps/blockly/blockly-icon.svg"
        descriptionImage: "qrc:/apps/blockly/description.jpg"
        descriptionTextFile: "qrc:/apps/blockly/desc.en.html"
        supportsGroups: false
        supportsWatchMode: false
    }

     ListElement {
         appId:"studio"
         name: "Aseba Studio"
         animatedIcon:"qrc:/apps/studio/studio-animated-icon.webp"
         icon: "qrc:/apps/studio/launcher-icon-studio.svg"
         descriptionImage: "qrc:/apps/studio/description.jpg"
         descriptionTextFile: "qrc:/apps/studio/desc.en.html"
         supportsGroups: true
         supportsWatchMode: true
     }
}
