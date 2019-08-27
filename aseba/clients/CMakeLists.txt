## text-based
if(NOT ANDROID)
    # text-based using QtCore
    add_subdirectory(massloader)
    set(CMAKE_CXX_STANDARD 17)

    # gui

    if (Qt5Widgets_FOUND)
        set(HAS_GRAPHICAL_CLIENTS ON)

        if(Qt5OpenGL_FOUND AND Qt5Xml_FOUND)
            set(HAS_STUDIO ON)
            add_subdirectory(qtcommon)
            add_subdirectory(studio)
            add_subdirectory(vpl)
        endif()


    endif()
    add_feature_info(THYMIO_CONFIG HAS_GRAPHICAL_CLIENTS "studio, event logger (depends on QtWidgets )")
    add_feature_info(STUDIO HAS_STUDIO "Studio")
    add_feature_info(JOY SDL2_FOUND "Aseba Joy ( depends on SDL )")
endif()

#add_subdirectory(vpl2)
