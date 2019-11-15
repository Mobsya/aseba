cmake_minimum_required(VERSION 3.0)
include(FindPackageHandleStandardArgs)

if(WIN32)
    #Bonjour.msi and Bonjour64.msi have been extracted from itunes
    configure_file(Bonjour.msi Bonjour.msi COPYONLY)
    configure_file(Bonjour64.msi Bonjour64.msi COPYONLY)
    #This variables are meant to be used by nsis - therefore they should use \ rather than /
    string(REPLACE "/" "\\" CMAKE_CURRENT_BINARY_DIR_WINDOWS "${CMAKE_CURRENT_BINARY_DIR}")
    set(BONJOUR64_MSI "${CMAKE_CURRENT_BINARY_DIR_WINDOWS}\\Bonjour.msi" CACHE STRING "Path of Bonjour.msi")
    set(BONJOUR_MSI   "${CMAKE_CURRENT_BINARY_DIR_WINDOWS}\\Bonjour64.msi" CACHE STRING "Path of Bonjour64.msi")
endif()
