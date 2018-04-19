TEMPLATE = subdirs


SUBDIRS =    \
  libusb     \
  qtzeroconf \
  app

libusb.subdir     = third_party/libusb
qtzeroconf.subdir = third_party/qtzeroconf
app.subdir        = src

app.depends = libusb qtzeroconf
