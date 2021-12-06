if (NOT BUILD_SHARED_LIBS)
    set(Boost_USE_STATIC_LIBS   ON)
endif()
find_package(Boost 1.73 COMPONENTS chrono system filesystem thread regex date_time program_options OPTIONAL_COMPONENTS python27)
add_definitions(-DBOOST_ALL_NO_LIB)
if(WIN32)
    add_definitions(-DBOOST_USE_WINDOWS_H)
endif()
if(NOT WIN32 AND NOT Boost_FOUND)
    include( ExternalProject )
    set( boost_URL "https://boostorg.jfrog.io/artifactory/main/release/1.73.0/source/boost_1_73_0.tar.gz" )
    set( boost_SHA256 "9995e192e68528793755692917f9eb6422f3052a53c5e13ba278a228af6c7acf" )
    set( boost_bootstrap ./bootstrap.sh)
    set( boost_b2 ./b2 )
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
                --prefix=${boost_INSTALL}
            BUILD_COMMAND
            ${boost_b2} install cxxflags=-fPIC cflags=-fPIC link=static variant=release threading=multi runtime-link=static
                --with-chrono --with-system --with-thread --with-date_time --with-regex --with-serialization --with-program_options --with-filesystem --with-python
            INSTALL_COMMAND ""
            INSTALL_DIR ${boost_INSTALL}

            BUILD_BYPRODUCTS
                    ${boost_LIB_DIR}/libboost_chrono${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_date_time${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_system${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_program_options${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_filesystem${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_regex${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_thread${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_serialization${boost_LIBRARY_SUFFIX}
                    ${boost_LIB_DIR}/libboost_python27${boost_LIBRARY_SUFFIX}
            )

    file(MAKE_DIRECTORY ${boost_INCLUDE_DIR})

    add_library(  Boost::boost STATIC IMPORTED )
    set_property( TARGET Boost::boost PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( Boost::boost external_boost )

    add_library(  Boost::chrono STATIC IMPORTED )
    set_property( TARGET Boost::chrono PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_chrono${boost_LIBRARY_SUFFIX} )
    set_property( TARGET Boost::chrono PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( Boost::chrono external_boost )

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

    add_library( Boost::python27 STATIC IMPORTED )
    set_property( TARGET Boost::python27 PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_python27${boost_LIBRARY_SUFFIX} )
    set_property( TARGET Boost::python27 PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( Boost::python27 external_boost )

    add_library( Boost::serialization STATIC IMPORTED )
    set_property( TARGET Boost::serialization PROPERTY IMPORTED_LOCATION ${boost_LIB_DIR}/libboost_serialization${boost_LIBRARY_SUFFIX} )
    set_property( TARGET Boost::serialization PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${boost_INCLUDE_DIR} )
    add_dependencies( Boost::serialization external_boost )
endif()

add_library(Boost::asio IMPORTED INTERFACE)
set_property(TARGET Boost::asio PROPERTY INTERFACE_LINK_LIBRARIES Boost::system Boost::regex)
add_library( Boost::beast IMPORTED INTERFACE)
set_property(TARGET Boost::beast PROPERTY INTERFACE_LINK_LIBRARIES Boost::asio)

