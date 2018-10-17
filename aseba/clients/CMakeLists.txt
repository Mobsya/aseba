## text-based
if(NOT ANDROID)
    # text-based using QtCore
    add_subdirectory(massloader)
    add_subdirectory(cmd)
    set(CMAKE_CXX_STANDARD 17)

    # gui

    if (Qt5Widgets_FOUND)
        if (Qt5Concurrent_FOUND AND Qt5Network_FOUND AND Qt5Xml_FOUND)
            add_subdirectory(thymioupgrader)
            set(HAS_THYMIO_UPGRADER ON)
        endif()


        add_subdirectory(thymiownetconfig)
        set(HAS_GRAPHICAL_CLIENTS ON)

        if(Qt5OpenGL_FOUND AND Qt5Xml_FOUND)
            set(HAS_STUDIO ON)
            add_subdirectory(studio)
        endif()


    endif()
    add_feature_info(THYMIO_CONFIG HAS_GRAPHICAL_CLIENTS "studio thymio config/upgrader, event logger (depends on QtWidgets )")
    add_feature_info(THYMIO_CONFIG HAS_THYMIO_UPGRADER   "upgrader (depends on QtConcurrent )")
    add_feature_info(STUDIO HAS_STUDIO "Studio")
    add_feature_info(JOY SDL2_FOUND "Aseba Joy ( depends on SDL )")
endif()

#add_subdirectory(vpl2)
