# Meson build file for libuvc subproject
project('libuvc', 'c',
  version: '0.0.7',
  license: 'BSD-3-Clause',
)

cc = meson.get_compiler('c')

# Dependencies
libusb_dep = dependency('libusb-1.0', required: true)
threads_dep = dependency('threads', required: true)

# Optional JPEG support
jpeg_dep = dependency('libjpeg', required: false)

# Source files
libuvc_sources = files(
  'src/ctrl.c',
  'src/ctrl-gen.c',
  'src/device.c',
  'src/diag.c',
  'src/frame.c',
  'src/init.c',
  'src/misc.c',
  'src/stream.c',
)

if jpeg_dep.found()
  libuvc_sources += files('src/frame-mjpeg.c')
endif

# Include directories
libuvc_inc = include_directories('include', 'include/libuvc')

# Generate config header
conf_data = configuration_data()
conf_data.set('LIBUVC_VERSION_MAJOR', 0)
conf_data.set('LIBUVC_VERSION_MINOR', 0)
conf_data.set('LIBUVC_VERSION_PATCH', 7)
conf_data.set('LIBUVC_HAS_JPEG', jpeg_dep.found())

configure_file(
  input: 'include/libuvc/libuvc_config.h.in',
  output: 'libuvc_config.h',
  configuration: conf_data,
)

# Build library
libuvc_deps = [libusb_dep, threads_dep]
if jpeg_dep.found()
  libuvc_deps += jpeg_dep
endif

libuvc_lib = library('uvc',
  libuvc_sources,
  include_directories: libuvc_inc,
  dependencies: libuvc_deps,
  install: true,
)

# Create dependency for use by parent project
libuvc_dep = declare_dependency(
  include_directories: libuvc_inc,
  link_with: libuvc_lib,
)

