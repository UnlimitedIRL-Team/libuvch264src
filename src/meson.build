library_name = 'gstlibuvch264src'

sources = [
  'gstlibuvch264src.c',
  'gstlibuvch264src.h',
]

shared_library(library_name, sources,
  dependencies: [gst_dep, gst_base_dep, libuvc_dep, libusb_dep],
  install: true,
  install_dir: join_paths(get_option('libdir'), 'gstreamer-1.0')
)
