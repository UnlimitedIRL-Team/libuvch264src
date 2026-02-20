This is a gstreamer plugin developed by UnlimitedIRL to support pulling H264 frames from DJI action cameras

We recommend looking at the BELABOX fork for up-to-date optimizations that may not be merged. https://github.com/BELABOX/gstlibuvch264src

For Rockchip decode on kernel 5.10 use mppvideodec

for Rockchip decode on kernel 6.6 use v4l2slh264dec

Example pipeline to send frames to HDMI output:

```
gst-launch-1.0 libuvch264src index=0 ! video/x-h264,width=1920,height=1080,framerate=30/1 ! queue ! h264parse ! queue ! v4l2slh264dec ! queue ! videoconvert ! kmssink
```

# Installation guide

## Patch libuvc
libuvc library does not support UVC 1.5 which is required to pull H264 frames from DJI action cameras. A patch is required to bypass the UVC 1.5 check and allow the camera to be used as a standard UVC device. 
The patch is included in this repository and can be applied to the libuvc source code and compiled/installed in this way:

```
# Prepare temporary buildfolder
mkdir /tmp/libuvc
cd /tmp/libuvc

# Download libuvc source code
git clone https://github.com/libuvc/libuvc.git .

# Ensure we are on the correct version (the patch is only tested with v0.0.7)
git checkout v0.0.7

# Download and apply the patch
wget https://raw.githubusercontent.com/UnlimitedIRL-Team/libuvch264src/refs/heads/dev/patches/uvc15-support.patch
patch -p1 < uvc15-support.patch

# Build and install the patched libuvc
cmake .
make
sudo make install
```

## Install the plugin
```
# Create temporary buildfolder
mkdir build

# Compile the plugin binary
meson setup build
meson compile -C build

# Move plugin from buildfolder to plugin-folder
sudo mv build/libgstlibuvch264src.so /lib/aarch64-linux-gnu/gstreamer-1.0/

```
