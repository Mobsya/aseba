set(LIB_USB_DIR "${CMAKE_CURRENT_LIST_DIR}/libusb/libusb/")
add_library(libusb STATIC
    ${LIB_USB_DIR}/core.c
    ${LIB_USB_DIR}/descriptor.c
    ${LIB_USB_DIR}/hotplug.c
    ${LIB_USB_DIR}/io.c
    ${LIB_USB_DIR}/sync.c
    ${LIB_USB_DIR}/strerror.c
)
if(UNIX)
   target_compile_definitions(libusb PRIVATE
       HAVE_SYS_TIME_H
       HAVE_POLL_H
       HAVE_DLFCN_H
       HAVE_DECL_TFD_NONBLOCK
       HAVE_INTTYPES_H
       HAVE_MEMORY_H
       HAVE_STDINT_H
       HAVE_STDLIB_H
       HAVE_STRINGS_H
       HAVE_STRING_H
       HAVE_STRUCT_TIMESPEC
       HAVE_SYS_STAT_H
       HAVE_SYS_TYPES_H
       THREADS_POSIX
       POLL_NFDS_TYPE=nfds_t
       HAVE_UNISTD_H
       _GNU_SOURCE)


   target_sources(libusb PRIVATE
       ${LIB_USB_DIR}/os/poll_posix.c
       ${LIB_USB_DIR}/os/threads_posix.c
   )

   if(ANDROID)
        target_sources(libusb PRIVATE
                ${LIB_USB_DIR}/os/linux_usbfs.c
                ${LIB_USB_DIR}/os/linux_netlink.c
                )
        target_compile_definitions(libusb PRIVATE OS_LINUX HAVE_PIPE2)
   endif()


   if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
       target_sources(libusb PRIVATE
           ${LIB_USB_DIR}/os/linux_usbfs.c
           ${LIB_USB_DIR}/os/linux_netlink.c
           )
       set(THREADS_PREFER_PTHREAD_FLAG ON)
       find_package(Threads REQUIRED)
       target_link_libraries(libusb Threads::Threads)
       target_compile_definitions(libusb PRIVATE -DUSBI_TIMERFD_AVAILABLE OS_LINUX HAVE_PIPE2)

       # It important not to require udev as it can't compile
       # on flatpack
       find_library(udev UDEV_FOUND)
       if(UDEV_FOUND)
           target_sources(libusb PRIVATE
               ${LIB_USB_DIR}/os/linux_udev.c
               )
           target_link_libraries(libusb udev)
           target_compile_definitions(libusb PRIVATE -DHAVE_LIBUDEV -DHAVE_LIBUDEV_H -DUSE_UDEV)
       endif(UDEV_FOUND)
   endif()

   if(APPLE)
       find_library(ObjC objc)
       target_link_libraries(libusb ${ObjC})
       target_sources(libusb PRIVATE ${LIB_USB_DIR}/os/darwin_usb.c)
       target_compile_definitions(libusb PRIVATE OS_DARWIN)
   endif()
endif()

if(WIN32)
    target_sources(libusb PRIVATE
        ${LIB_USB_DIR}/os/windows_nt_common.c
        ${LIB_USB_DIR}/os/windows_usbdk.c
        ${LIB_USB_DIR}/os/windows_winusb.c
        ${LIB_USB_DIR}/os/threads_windows.c
        ${LIB_USB_DIR}/os/poll_windows.c
        )
    target_compile_definitions(libusb PRIVATE OS_WINDOWS -DPOLL_NFDS_TYPE=unsigned)
endif()

target_include_directories(libusb PRIVATE ${LIB_USB_DIR})
target_include_directories(libusb PUBLIC "${CMAKE_CURRENT_LIST_DIR}/libusb/" "${CMAKE_CURRENT_LIST_DIR}/include")

