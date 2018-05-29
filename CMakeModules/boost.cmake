find_package(Boost 1.67 COMPONENTS system filesystem thread regex date_time program_options)
if(NOT Boost_FOUND)
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
            ${boost_b2} install link=static variant=release threading=multi runtime-link=static --with-system --with-thread --with-date_time --with-regex --with-serialization --with-program_options
            INSTALL_COMMAND ""
            INSTALL_DIR ${boost_INSTALL}

            BUILD_BYPRODUCTS
                    ${boost_LIB_DIR}/libboost_date_time${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_system${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_program_options${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_filesystem${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_regex${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_thread${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_serialization${boost_LIBRARY_SUFFIX}
            )

    file(MAKE_DIRECTORY ${boost_INCLUDE_DIR})


    add_library( Boost::date_time STATIC IMPORTED )
    set_property( TARGET Boost::date_time PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_date_time${boost_LIBRARY_SUFFIX} )
    set_property( TARGET Boost::date_time PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( Boost::date_time external_boost )

    add_library( Boost::system STATIC IMPORTED )
    set_property( TARGET Boost::system PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_system${boost_LIBRARY_SUFFIX} )
    set_property( TARGET Boost::system PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( Boost::system external_boost )

    add_library( Boost::filesystem STATIC IMPORTED )
    set_property( TARGET Boost::filesystem PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_filesystem${boost_LIBRARY_SUFFIX} )
    set_property( TARGET Boost::filesystem PROPERTY INTERFACE_LINK_LIBRARIES Boost::system )
    set_property( TARGET Boost::filesystem PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( Boost::filesystem external_boost )

    add_library( Boost::program_options STATIC IMPORTED )
    add_dependencies( Boost::program_options external_boost )
    set_property( TARGET Boost::program_options PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_program_options${boost_LIBRARY_SUFFIX} )
    set_property( TARGET Boost::program_options PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )


    add_library( Boost::regex STATIC IMPORTED )
    set_property( TARGET Boost::regex PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_regex${boost_LIBRARY_SUFFIX} )
    set_property( TARGET Boost::regex PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( Boost::regex external_boost )

    add_library( Boost::thread STATIC IMPORTED )
    set_property( TARGET Boost::thread PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_thread${boost_LIBRARY_SUFFIX} )
    set_property( TARGET Boost::thread PROPERTY INTERFACE_LINK_LIBRARIES Boost::system )
    set_property( TARGET Boost::thread PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( Boost::thread external_boost )

#     add_library( Boost::python STATIC IMPORTED )
#     set_property( TARGET Boost::python PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_python${boost_LIBRARY_SUFFIX} )
#     set_property( TARGET Boost::python PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
#     add_dependencies( Boost::python external_boost )

    add_library( Boost::serialization STATIC IMPORTED )
    set_property( TARGET Boost::serialization PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_serialization${boost_LIBRARY_SUFFIX} )
    set_property( TARGET Boost::serialization PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( Boost::serialization external_boost )
endif()


add_library(Boost::asio IMPORTED INTERFACE)
set_property(TARGET Boost::asio PROPERTY INTERFACE_LINK_LIBRARIES Boost::system Boost::regex)
add_library( Boost::beast IMPORTED INTERFACE)
set_property(TARGET Boost::beast PROPERTY INTERFACE_LINK_LIBRARIES Boost::asio)

