TEMPLATE = subdirs


SUBDIRS =    \
  qtzeroconf \
  app

qtzeroconf.subdir = $$absolute_path($PWD/../../../../third_party/qtzeroconf)
app.subdir        = src

app.depends = qtzeroconf
