import QtQuick 2.12
import QtQuick.Controls 2.12

StackView {
    id: main_layout
    anchors.fill: parent

    function out_of_frame_pos() {
        return (main_layout.mirrored ? 1 : -1) * -main_layout.height
    }

    property  int anim_duration: 250

    popEnter: Transition {
        YAnimator {
           from: 0
           to: 0
           duration: main_layout.anim_duration
       }
    }
    popExit: Transition {
        YAnimator {
            from: 0
            to: main_layout.out_of_frame_pos()
            duration: main_layout.anim_duration
            easing.type: Easing.OutCubic
        }
    }

    pushEnter: Transition {
       YAnimator {
           from: main_layout.out_of_frame_pos()
           to: 0
           duration: main_layout.anim_duration
           easing.type: Easing.InCubic
       }
    }

    pushExit: Transition {
        YAnimator {
            from: 0
            to: 0
            duration: main_layout.anim_duration
        }
    }
}