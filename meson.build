project('libuvch264src', 'c',
  version: '1.0',
  default_options: ['warning_level=2', 'buildtype=debugoptimized']
)

# Dependencies
gst_dep = dependency('gstreamer-1.0', version: '>=1.14')
gst_base_dep = dependency('gstreamer-base-1.0', version: '>=1.14')
libusb_dep = dependency('libusb-1.0', required: true)

# Try system libuvc first (which should be available after CMake build), skip subproject
libuvc_dep = dependency('libuvc', required: true)

subdir('src')
