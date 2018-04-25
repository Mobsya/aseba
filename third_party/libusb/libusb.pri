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

SOURCES += \
  $$PWD/libusb/libusb/os/linux_usbfs.c \
  $$PWD/libusb/libusb/os/poll_posix.c \
  $$PWD/libusb/libusb/os/threads_posix.c \
  $$PWD/libusb/libusb/os/linux_netlink.c

linux:!android {
  SOURCES += $$PWD/libusb/libusb/os/linux_udev.c
  DEFINES += HAVE_LIBUDEV HAVE_LIBUDEV_H USBI_TIMERFD_AVAILABLE USE_UDEV
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

SOURCES +=
