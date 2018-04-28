find_package(Boost 1.67 COMPONENTS system filesystem thread regex python date_time program_options)
if(Boost_FOUND)
    add_library(boost_meta INTERFACE)
    target_include_directories(boost_meta INTERFACE ${BOOST_INCLUDE_DIRS})
    target_compile_definitions(boost_meta INTERFACE ${Boost_DEFINITIONS})
    add_library( boost::system IMPORTED INTERFACE)
    set_property(TARGET boost::system PROPERTY INTERFACE_LINK_LIBRARIES ${Boost_SYSTEM_LIBRARY} boost_meta)
    add_library( boost::filesystem IMPORTED INTERFACE)
    set_property(TARGET boost::filesystem PROPERTY INTERFACE_LINK_LIBRARIES ${Boost_FILESYSTEM_LIBRARY} boost_meta boost::system)
    add_library( boost::date_time IMPORTED INTERFACE)
    set_property(TARGET boost::date_time PROPERTY INTERFACE_LINK_LIBRARIES ${Boost_DATE_TIME_LIBRARY} boost_meta)
    add_library( boost::program_options IMPORTED INTERFACE)
    set_property(TARGET boost::program_options PROPERTY INTERFACE_LINK_LIBRARIES ${Boost_PROGRAM_OPTIONS_LIBRARY} boost_meta)
    add_library( boost::regex IMPORTED INTERFACE)
    set_property(TARGET boost::regex PROPERTY INTERFACE_LINK_LIBRARIES ${Boost_REGEX_LIBRARY} boost_meta)
    add_library( boost::thread IMPORTED INTERFACE)
    set_property(TARGET boost::thread PROPERTY INTERFACE_LINK_LIBRARIES ${Boost_THREAD_LIBRARY} boost_meta boost::system)
    add_library( boost::python IMPORTED INTERFACE)
    set_property(TARGET boost::python PROPERTY INTERFACE_LINK_LIBRARIES ${Boost_PYTHON_LIBRARY} boost_meta)
    add_library( boost::serialization IMPORTED INTERFACE)
    set_property(TARGET boost::serialization PROPERTY INTERFACE_LINK_LIBRARIES ${Boost_SERIALIZATION_LIBRARY} boost_meta)
else()
    include( ExternalProject )

    if(WIN32)
        set( boost_URL "https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.zip" )
        set( boost_SHA256 "7e37372d8cedd0fd6b7529e9dd67c2cb1c60e6c607aed721f5894d704945a7ec" )
        set( boost_bootstrap cmd /C bootstrap.bat msvc)
        set( boost_b2 b2.exe )
    else()
    set( boost_URL "https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.bz2" )
    set( boost_SHA256 "2684c972994ee57fc5632e03bf044746f6eb45d4920c343937a465fd67a5adba" )
    set( boost_bootstrap ./bootstrap.sh)
    set( boost_b2 ./b2 )
    endif()
    set( boost_INSTALL ${CMAKE_BINARY_DIR}/third_party/boost )
    set( boost_INCLUDE_DIR ${boost_INSTALL}/include )
    set( boost_LIB_DIR ${boost_INSTALL}/lib )

    set( boost_LIBRARY_SUFFIX .a )

    ExternalProject_Add( external_boost
            PREFIX boost
            URL ${boost_URL}
            URL_HASH SHA256=${boost_SHA256}
            BUILD_IN_SOURCE 1
            CONFIGURE_COMMAND ${boost_bootstrap}
                --with-libraries=filesystem
                --with-libraries=system
                --with-libraries=date_time
                --prefix=${boost_INSTALL}
            BUILD_COMMAND
            ${boost_b2} install link=static variant=release threading=multi runtime-link=static --with-system --with-thread --with-date_time --with-regex --with-serialization --with-program_options --with-python
            INSTALL_COMMAND ""
            INSTALL_DIR ${boost_INSTALL}

            BUILD_BYPRODUCTS
                    ${boost_LIB_DIR}/libboost_date_time${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_system${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_program_options${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_filesystem${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_regex${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_thread${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_python${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_serialization${boost_LIBRARY_SUFFIX}
            )

    file(MAKE_DIRECTORY ${boost_INCLUDE_DIR})


    add_library( boost::date_time STATIC IMPORTED )
    set_property( TARGET boost::date_time PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_date_time${boost_LIBRARY_SUFFIX} )
    set_property( TARGET boost::date_time PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( boost::date_time external_boost )

    add_library( boost::system STATIC IMPORTED )
    set_property( TARGET boost::system PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_system${boost_LIBRARY_SUFFIX} )
    set_property( TARGET boost::system PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( boost::system external_boost )

    add_library( boost::filesystem STATIC IMPORTED )
    set_property( TARGET boost::filesystem PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_filesystem${boost_LIBRARY_SUFFIX} )
    set_property( TARGET boost::filesystem PROPERTY INTERFACE_LINK_LIBRARIES boost::system )
    set_property( TARGET boost::filesystem PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( boost::filesystem external_boost )

    add_library( boost::program_options STATIC IMPORTED )
    add_dependencies( boost::program_options external_boost )
    set_property( TARGET boost::program_options PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_program_options${boost_LIBRARY_SUFFIX} )
    set_property( TARGET boost::program_options PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )


    add_library( boost::regex STATIC IMPORTED )
    set_property( TARGET boost::regex PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_regex${boost_LIBRARY_SUFFIX} )
    set_property( TARGET boost::regex PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( boost::regex external_boost )

    add_library( boost::thread STATIC IMPORTED )
    set_property( TARGET boost::thread PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_thread${boost_LIBRARY_SUFFIX} )
    set_property( TARGET boost::thread PROPERTY INTERFACE_LINK_LIBRARIES boost::system )
    set_property( TARGET boost::thread PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( boost::thread external_boost )

    add_library( boost::python STATIC IMPORTED )
    set_property( TARGET boost::python PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_python${boost_LIBRARY_SUFFIX} )
    set_property( TARGET boost::python PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( boost::python external_boost )

    add_library( boost::serialization STATIC IMPORTED )
    set_property( TARGET boost::serialization PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_serialization${boost_LIBRARY_SUFFIX} )
    set_property( TARGET boost::serialization PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( boost::serialization external_boost )
endif()

add_library(boost::asio IMPORTED INTERFACE)
set_property(TARGET boost::asio PROPERTY INTERFACE_LINK_LIBRARIES boost::system boost::regex)
add_library( boost::beast IMPORTED INTERFACE)
set_property(TARGET boost::beast PROPERTY INTERFACE_LINK_LIBRARIES boost::asio)

