project('libuvch264src', 'c',
  version: '1.0',
  default_options: ['warning_level=2', 'buildtype=debugoptimized']
)

# Dependencies
gst_dep = dependency('gstreamer-1.0', version: '>=1.14')
gst_base_dep = dependency('gstreamer-base-1.0', version: '>=1.14')
libusb_dep = dependency('libusb-1.0', required: true)

# Try system libuvc first, fall back to subproject with patches
libuvc_dep = dependency('libuvc', required: false)

if not libuvc_dep.found()
  message('System libuvc not found, building from source with UVC 1.5 patch...')
  libuvc_proj = subproject('libuvc')
  libuvc_dep = libuvc_proj.get_variable('libuvc_dep')
endif

subdir('src')
