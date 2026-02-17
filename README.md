This is a gstreamer plugin developed by UnlimitedIRL to support pulling H264 frames from DJI action cameras  

We recommend looking at the BELABOX fork for up-to-date optimizations that may not be merged. https://github.com/BELABOX/gstlibuvch264src

For Rockchip decode on kernel 5.10 use mppvideodec

for Rockchip decode on kernel 6.6 use v4l2slh264dec

Example pipeline to send frames to HDMI output: 

```bash
gst-launch-1.0 libuvch264src index=0 ! video/x-h264,width=1920,height=1080,framerate=30/1 ! queue ! h264parse ! queue ! v4l2slh264dec ! queue ! videoconvert ! kmssink
```

## Build Steps

### Dependencies

```bash
# Debian/Ubuntu
sudo apt install build-essential meson pkg-config git
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
sudo apt install libusb-1.0-0-dev libjpeg-dev

# macOS
brew install meson pkg-config gstreamer gst-plugins-base libusb jpeg
```

### Build

The build system will automatically download and patch libuvc from upstream if not installed on the system.

```bash
cd libuvch264src
meson setup build
meson compile -C build
sudo meson install -C build
```

### libuvc Patches

This project applies the following patches to upstream libuvc v0.0.7:
- **UVC 1.5 support** (`uvc15-support.patch`): Adds support for UVC 1.5 specification devices

Patches are located in `libuvch264src/subprojects/libuvc-patches/`

### Manual Installation (if needed)

```bash
# Move plugin to GStreamer plugin directory (adjust path for your architecture)
sudo mv /usr/local/lib/$(uname -m)-linux-gnu/gstreamer-1.0/libgstlibuvch264src.so /lib/$(uname -m)-linux-gnu/gstreamer-1.0/
```

