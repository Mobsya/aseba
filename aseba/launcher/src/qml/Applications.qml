import QtQuick 2.0

ListModel {
    id: appsModel
    
    ListElement { 
        name: "VPL"
        animatedIcon:"qrc:/apps/vpl/vpl-animated-icon.webp"
    }
    ListElement { 
        name: "Scatch"
        animatedIcon:"qrc:/apps/scratch/scratch-animated-icon.webp"
    }
    
    ListElement { 
        name: "Blockly"
        animatedIcon: "qrc:/apps/blockly/blockly-animated-icon.webp"
    }
    
    ListElement { 
        name: "Studio"
        animatedIcon:"qrc:/apps/studio/studio-animated-icon.webp"
    }  
}
