## text-based
if(NOT ANDROID)
    # text-based using QtCore
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
    add_feature_info(STUDIO HAS_STUDIO "Studio")
endif()

#add_subdirectory(vpl2)
