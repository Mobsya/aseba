TEMPLATE = subdirs


SUBDIRS =    \
  qtzeroconf \
  libusb \
  app

qtzeroconf.subdir = $$absolute_path($PWD/../../../../third_party/qtzeroconf)
libusb.subdir = $$absolute_path($PWD/../../../../third_party/libusb)
app.subdir        = src
app.depends = qtzeroconf libusb
