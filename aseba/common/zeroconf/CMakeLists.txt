add_library(asebazeroconf STATIC
	zeroconf.cpp
	txtrecord.cpp
	target.cpp
	zeroconf-thread.cpp
	zeroconf-dashelhub.cpp
)
set_target_properties(asebazeroconf PROPERTIES VERSION ${LIB_VERSION_STRING})
target_link_libraries(asebazeroconf PUBLIC asebacommon zeroconf asebadashelplugins)


# Zeroconf with Qt
if (Qt5Network_FOUND)
	set(CMAKE_AUTOMOC ON)
    add_library(asebazeroconfqt STATIC zeroconf-qt.cpp)
	set_target_properties(asebazeroconfqt PROPERTIES VERSION ${LIB_VERSION_STRING})
	target_include_directories(asebazeroconfqt PUBLIC ${DNSSD_INCLUDE_DIRS})
	target_link_libraries(asebazeroconfqt PUBLIC asebazeroconf Qt5::Core Qt5::Network)

endif ()
add_feature_info(QT_ZEROCONF Qt5Network_FOUND "Qt Zeroconf library ( depend on QtNetwork )")


