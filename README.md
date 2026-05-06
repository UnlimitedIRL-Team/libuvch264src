# libuvch264src

A GStreamer source plugin for pulling H.264-encoded video from UVC cameras over USB, with specific workarounds for DJI action cameras (Osmo Action 4, Osmo Pocket 3, etc.).

> **Note: this is the `experimental` branch.** It carries fixes that have not yet landed on `main` and are still being validated in the field. It may be more unstable than `main` — expect rough edges, log spam, and the occasional regression. If you don't specifically need the workarounds described below, use `main` instead.

## What this plugin solves for DJI cameras

DJI action cameras can expose an H.264 stream over USB when switched into UVC mode, but they don't behave like a typical UVC webcam. This plugin works around three separate issues that otherwise make the cameras unusable, especially on Rockchip SBCs:

### 1. UVC 1.5 protocol version
Stock `libuvc` rejects DJI cameras because they advertise UVC version 1.5, which libuvc's parser does not recognize. We ship a patch (`patches/uvc15-support.patch`) that adds UVC 1.5 to the accepted version list and auto-detaches the kernel `uvcvideo` driver during `uvc_wrap()`. Without this patch the camera fails to open.

### 2. Wrong SPS dimensions on Pocket 3 at 4K
The DJI Osmo Pocket 3 streams 3840×2160 H.264 but its in-band SPS NAL declares 1920×1080. Hardware decoders — notably Rockchip rkvdec2 on RK3566 — allocate buffers from the SPS, then time out on every frame because the picture is four times larger than the buffer they planned for. The plugin parses the negotiated caps, compares them to the SPS dimensions, and rewrites the SPS (including frame cropping) to match the real resolution before forwarding the IDR downstream. Cameras that advertise correct dimensions are unaffected — the rewriter is a no-op when the SPS already agrees with the negotiated caps.

### 3. Corrupt frames during camera startup
For roughly 0.3 seconds after the camera switches from RNDIS to UVC mode, DJI cameras emit H.264 frames with corrupt macroblock data. On RK3566 these frames hang the VPU (hardware timeout, IOMMU corruption — essentially fatal). The plugin includes a configurable warmup gate that silently drops a number of UVC frames at stream start, then forces a fresh IDR with valid SPS/PPS once the gate opens.

## Properties

| Property          | Type            | Default | Description |
|-------------------|-----------------|---------|-------------|
| `index`           | string          | `"0"`   | Positional device index used when `bus` / `device-address` are not set. Selects the Nth UVC device returned by libuvc enumeration. **Unreliable when multiple identical-VID:PID cameras are attached** — see below. |
| `bus`             | int (-1..255)   | `-1`    | USB bus number. When set together with `device-address`, the plugin opens `/dev/bus/usb/<bus>/<addr>` directly via `uvc_wrap()` and bypasses libuvc enumeration entirely. `-1` disables this code path. |
| `device-address`  | int (-1..255)   | `-1`    | USB device address on the bus. See `bus`. |
| `warmup-frames`   | int (0..600)    | `90`    | Number of UVC frames to drop at stream start, to skip past the post-mode-switch corruption window. At 30 fps the default is ~3 seconds. Set to `0` to disable the warmup gate (e.g. for cameras that don't have the issue). |

`gst-inspect-1.0 libuvch264src` will list these once the plugin is installed.

## Quick start

Send frames to HDMI output on Rockchip kernel 6.6:

```
gst-launch-1.0 libuvch264src index=0 ! video/x-h264,width=1920,height=1080,framerate=30/1 ! queue ! h264parse ! queue ! v4l2slh264dec ! queue ! videoconvert ! kmssink
```

For Rockchip kernel 5.10, replace `v4l2slh264dec` with `mppvideodec`.

For DJI Pocket 3 in 4K mode (the SPS rewriter kicks in automatically):

```
gst-launch-1.0 libuvch264src index=0 ! video/x-h264,width=3840,height=2160,framerate=30/1 ! queue ! h264parse ! queue ! v4l2slh264dec ! queue ! videoconvert ! kmssink
```

## Multiple cameras with identical VID:PID

When two cameras share VID:PID (e.g. two DJI Osmo Action 4 units, both `2ca3:0023`), the `index` property is unreliable: libusb's enumeration order is not guaranteed to be stable, so two pipelines using `index=0` and `index=1` may both resolve to the same physical device, or swap between launches. One camera ends up frozen, the other gets two readers fighting over it.

Use `bus` and `device-address` instead. These are read from the kernel and select the camera by its physical USB location.

**Step 1:** Find each camera's bus and device address:

```
lsusb | grep 2ca3
# Bus 003 Device 002: ID 2ca3:0023 ...
# Bus 005 Device 003: ID 2ca3:0023 ...
```

**Step 2:** Launch pipelines with `bus` and `device-address`:

```
# Camera A
gst-launch-1.0 libuvch264src bus=3 device-address=2 ! video/x-h264,width=1920,height=1080,framerate=30/1 ! queue ! h264parse ! queue ! v4l2slh264dec ! queue ! videoconvert ! kmssink

# Camera B
gst-launch-1.0 libuvch264src bus=5 device-address=3 ! video/x-h264,width=1920,height=1080,framerate=30/1 ! queue ! h264parse ! queue ! v4l2slh264dec ! queue ! videoconvert ! kmssink
```

Notes:

- `device-address` changes when a camera is unplugged and replugged; `bus` is tied to the physical USB controller/port and stays stable.
- This code path requires read/write access to `/dev/bus/usb/<bus>/<addr>` — typically root, or a udev rule that grants the running user access.
- When `bus`/`device-address` are set, the SPS/PPS cache key is derived from `<bus>-<addr>` instead of `index`, so each physical camera gets its own cache file under `/tmp/spspps/`.
- Use `GST_DEBUG=libuvch264src:4` to see device-selection details at startup.

# Installation

## 1. Patch and install libuvc

libuvc upstream does not support UVC 1.5. Apply the included patch and install:

```
mkdir /tmp/libuvc
cd /tmp/libuvc

# Download libuvc source
git clone https://github.com/libuvc/libuvc.git .

# The patch is tested only against v0.0.7
git checkout v0.0.7

# Download and apply the patch
wget https://raw.githubusercontent.com/UnlimitedIRL-Team/libuvch264src/refs/heads/experimental/patches/uvc15-support.patch
patch -p1 < uvc15-support.patch

# Build and install
cmake .
make
sudo make install
```

> **Heads-up:** `sudo make install` will overwrite a system-installed `libuvc` if one is present. Other applications that link against `libuvc` will pick up the patched version after this step. The patch is additive (it accepts UVC 1.5 in addition to 1.0/1.1, and enables auto-detach in `uvc_wrap()`), so this is normally harmless, but worth knowing before you run it.

## 2. Build the plugin

```
meson setup build
meson compile -C build

# Install into the GStreamer plugin directory.
# The path below is for aarch64 Debian/Ubuntu — adjust for your distro.
sudo mv build/src/libgstlibuvch264src.so /lib/aarch64-linux-gnu/gstreamer-1.0/
```

To find the right install directory for your system:

```
gst-inspect-1.0 --print-plugin-paths 2>/dev/null || pkg-config --variable=pluginsdir gstreamer-1.0
```

Verify the plugin loaded:

```
gst-inspect-1.0 libuvch264src
```

## See also

For optimizations and fixes not yet integrated here, see the BELABOX fork: https://github.com/BELABOX/gstlibuvch264src
