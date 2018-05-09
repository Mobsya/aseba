TARGET = usb

# Input
SOURCES += \
  $$PWD/libusb/libusb/core.c \
  $$PWD/libusb/libusb/descriptor.c \
  $$PWD/libusb/libusb/hotplug.c \
  $$PWD/libusb/libusb/io.c \
  $$PWD/libusb/libusb/sync.c \
  $$PWD/libusb/libusb/strerror.c


unix {

DEFINES += \
    HAVE_SYS_TIME_H \
    HAVE_POLL_H \
    HAVE_DLFCN_H \
    HAVE_DECL_TFD_NONBLOCK \
    HAVE_INTTYPES_H \
    HAVE_MEMORY_H \
    HAVE_STDINT_H \
    HAVE_STDLIB_H \
    HAVE_STRINGS_H \
    HAVE_STRING_H \
    HAVE_STRUCT_TIMESPEC \
    HAVE_SYS_STAT_H \
    HAVE_SYS_TYPES_H \
    THREADS_POSIX \
    POLL_NFDS_TYPE=nfds_t \
    HAVE_UNISTD_H \
    _GNU_SOURCE

SOURCES += \
  $$PWD/libusb/libusb/os/linux_usbfs.c \
  $$PWD/libusb/libusb/os/poll_posix.c \
  $$PWD/libusb/libusb/os/threads_posix.c \
  $$PWD/libusb/libusb/os/linux_netlink.c

linux:!android {
  SOURCES += $$PWD/libusb/libusb/os/linux_udev.c
  DEFINES += HAVE_LIBUDEV HAVE_LIBUDEV_H USBI_TIMERFD_AVAILABLE USE_UDEV
}


android {
    DEFINES += OS_LINUX HAVE_PIPE2
}

}

win32 {
  SOURCES += \
    $$PWD/libusb/libusb/os/windows_nt_common.c \
    $$PWD/libusb/libusb/os/windows_usbdk.c \
    $$PWD/libusb/libusb/os/windows_winusb.c \
    $$PWD/libusb/libusb/os/threads_windows.c \
    $$PWD/libusb/libusb/os/poll_windows.c
}

mac {
  SOURCES += \
    $$PWD/libusb/libusb/os/darwin_usb.c
}

INCLUDEPATH += $$PWD/libusb/libusb $$PWD/include
