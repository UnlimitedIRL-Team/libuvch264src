This is a gstreamer plugin developed by UnlimitedIRL to support pulling H264 frames from DJI action cameras  

We recommend looking at the BELABOX fork for up-to-date optimizations that may not be merged. https://github.com/BELABOX/gstlibuvch264src

For Rockchip decode on kernel 5.10 use mppvideodec

for Rockchip decode on kernel 6.6 use v4l2slh264dec

Example pipeline to send frames to HDMI output: 

```
gst-launch-1.0 libuvch264src index=0 ! video/x-h264,width=1920,height=1080,framerate=30/1 ! queue ! h264parse ! queue ! v4l2slh264dec ! queue ! videoconvert ! kmssink
```

## Build Steps

```
sudo apt install build-essential cmake git meson pkg-config
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
sudo apt install libusb-1.0-0 libusb-1.0-0-dev

cd libuvc
cmake .
make & sudo make install

cd ..
mkdir build
meson setup build libuvch264src/

cd build
meson compile
meson install

sudo mv /usr/local/lib/aarch64-linux-gnu/gstreamer-1.0/libgstlibuvch264src.so /lib/aarch64-linux-gnu/gstreamer-1.0/
sudo cp /usr/local/lib/libuvc.* /usr/lib/aarch64-linux-gnu/
```
