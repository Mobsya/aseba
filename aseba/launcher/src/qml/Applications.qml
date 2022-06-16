import QtQuick 2.12

ListModel {
    id: appsModel


    function launch_blockly(device) {
        const baseurl = Utils.webapp_base_url("blockly");
		const language = Utils.uiLanguage;
        if(!baseurl) {
            return false;
        }
        if ((language=="de")||(language=="en")||(language=="es")||(language=="fr")||(language=="it")||(language=="pl")||(language=="zh-cn")){
            const url = "%1/thymio_blockly.%2.html#device=%3&ws=%4&pass=%5"
                .arg(baseurl)
                .arg(language)
                .arg(device.id)
                .arg(device.websocketEndpoint())
                .arg(device.password())
            return Utils.openUrl(url);
        } else {
            const url = "%1/thymio_blockly.en.html#device=%2&ws=%3&pass=%4"
                .arg(baseurl)
                .arg(device.id)
                .arg(device.websocketEndpoint())
                .arg(device.password())
			return Utils.openUrl(url);
        }
    }

    function launch_scratch(device) {
        const baseurl = Utils.webapp_base_url("scratch");
        if(!baseurl) {
            return false;
        }
        const url = "%1/index.html?device=%2&ws=%3&pass=%4"
            .arg(baseurl).arg(device.id).arg(device.websocketEndpoint()).arg(device.password())
        return Utils.openUrl(url)
    }

    function launch_studio(device) {
        if(Utils.platformIsOsX()) {
            Utils.launchOsXBundle("AsebaStudio", {"uuid" : device.id,
                                      "endpoint" : device.tcpEndpoint(),
                                      "password" : device.password()})
        } else {
            var program = Utils.search_program("asebastudio")
            if(!program)
                return false;
            return Utils.launch_process(program, ["--uuid", device.id,
                                                  "--endpoint", device.tcpEndpoint(),
                                                  "--password", device.password()])
        }
    }

    function launch_vplClassic(device) {
        if(Utils.platformIsOsX()) {
            Utils.launchOsXBundle("ThymioVPLClassic", {"uuid" : device.id,
                                                       "endpoint" : device.tcpEndpoint(),
                                                       "password" : device.password()})
        } else {
            var program = Utils.search_program("thymiovplclassic")
            if(!program)
                return false;
            return Utils.launch_process(program, ["--uuid", device.id,
                                                  "--endpoint", device.tcpEndpoint(),
                                                  "--password", device.password()])
        }
    }

    function launch_vpl3(device) {
        const baseurl = Utils.webapp_base_url("vpl3");
        const language = Utils.uiLanguage
        if(!baseurl) {
            return false;
        }
        
        const url = "%1/index.html?robot=thymio-tdm&role=teacher&uilanguage=%2#uuid=%3&w=%4&pass=%5"
            .arg(baseurl)
            .arg(language)
            .arg(device.id)
            .arg(device.websocketEndpoint())
            .arg(device.password())
        return Utils.openUrl(url)
    }

    property var launch_functions : {
        "blockly" : launch_blockly,
        "scratch" : launch_scratch,
        "vplClassic" : launch_vplClassic,
        "studio" : launch_studio,
        "vpl3" : launch_vpl3
    }


    function launch_function(app) {
        const appId = app.appId
        console.log(appId)
        return launch_functions[appId]
    }

    Component.onCompleted: {
        create_application_list()
    }

    function create_application_list() {

        var applicationList = [
               {
                   appId:"vplClassic",
                   name: "VPL",
                   animatedIcon:"qrc:/apps/vpl/vpl-animated-icon.webp",
                   icon: "qrc:/apps/vpl/launcher-icon-vpl.svg",
                   descriptionImage: "qrc:/apps/vpl/description.jpg",
                   descriptionTextFile: "qrc:/apps/vpl/desc.%1.html",
                   supportsGroups: false,
                   supportsWatchMode: false,
                   supportsNonThymioDevices: false,
                   helpUrl: "https://www.thymio.org/%1/program/vpl/",
                   isIosSupported:false,
                   isAndroidSupported:false
                },
                {
                    appId:"vpl3",
                    name: "VPL 3",
                    animatedIcon:"qrc:/apps/vpl3/vpl3-animated-icon.webp",
                    icon: "qrc:/apps/vpl3/vpl_img.png",
                    descriptionImage: "qrc:/apps/vpl3/description.jpg",
                    descriptionTextFile: "qrc:/apps/vpl3/desc.%1.html",
                    supportsGroups: false,
                    supportsWatchMode: false,
                    helpUrl: "https://www.thymio.org/%1/program/vpl3/",
                    isIosSupported: true,
                    isAndroidSupported:true
                },
                {
                    appId: "scratch",
                    name: "Scratch",
                    animatedIcon:"qrc:/apps/scratch/scratch-animated-icon.webp",
                    icon: "qrc:/apps/scratch/launcher-icon-scratch.svg",
                    descriptionImage: "qrc:/apps/scratch/description.jpg",
                    descriptionTextFile: "qrc:/apps/scratch/desc.%1.html",
                    supportsGroups: false,
                    supportsWatchMode: false,
                    supportsNonThymioDevices: false,
                    helpUrl: "https://www.thymio.org/%1/program/scratch/",
                    isIosSupported:true,
                    isAndroidSupported:true
                 },

                {
                    appId: "blockly",
                    name: "Blockly",
                    animatedIcon: "qrc:/apps/blockly/blockly-animated-icon.webp",
                    icon: "qrc:/apps/blockly/blockly-icon.svg",
                    descriptionImage: "qrc:/apps/blockly/description.jpg",
                    descriptionTextFile: "qrc:/apps/blockly/desc.%1.html",
                    supportsGroups: false,
                    supportsWatchMode: false,
                    supportsNonThymioDevices: false,
                    helpUrl: "https://www.thymio.org/%1/program/blockly/",
                    isIosSupported:false,
                    isAndroidSupported:false
                 },

                {
                    appId:"studio",
                    name: "Aseba Studio",
                    animatedIcon:"qrc:/apps/studio/studio-animated-icon.webp",
                    icon: "qrc:/apps/studio/launcher-icon-studio.svg",
                    descriptionImage: "qrc:/apps/studio/description.jpg",
                    descriptionTextFile: "qrc:/apps/studio/desc.%1.html",
                    supportsGroups: true,
                    supportsWatchMode: true,
                    supportsNonThymioDevices: true,
                    helpUrl: "https://www.thymio.org/%1/program/aseba/",
                    isIosSupported:false,
                    isAndroidSupported:false
                 }
                ]

        if(Utils.platformIsIos())
        {
            applicationList =  applicationList.filter(function(app) {
                      return app.isIosSupported;});

        } else if(Utils.platformIsAndroid()) {
            applicationList =  applicationList.filter(function(app) {
                      return app.isAndroidSupported;});
        }
        append(applicationList)
    }
}
