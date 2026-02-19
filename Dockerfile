# Build environment for libuvch264src
# Based on Ubuntu 22.04 LTS for stable GStreamer and build tools

FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    meson \
    ninja-build \
    pkg-config \
    git \
    cmake \
    # GStreamer development packages
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-tools \
    # libuvc dependencies
    libusb-1.0-0-dev \
    libjpeg-dev \
    # Cleanup
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /src

# Copy source code
COPY . .

# Build the project
RUN rm -rf buildDir && \
    mkdir -p buildDir && \
    # First, build libuvc using its native CMake build system with patches applied \
    cd buildDir && \
    git clone https://github.com/libuvc/libuvc.git libuvc-build && \
    cd libuvc-build && \
    git checkout v0.0.7 && \
    # Apply the UVC 1.5 patch \
    patch -p1 < ../../patches/uvc15-support.patch && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLE=OFF -DBUILD_TEST=OFF && \
    make -j$(nproc) && \
    make install && \
    # Now build the main project \
    cd /src && \
    meson setup buildDir && \
    meson compile -C buildDir

# Default command: show build success and plugin info
CMD ["bash", "-c", "echo 'Build successful!' && echo '' && echo '🔌 Plugin information:' && GST_PLUGIN_PATH=/src/buildDir/src gst-inspect-1.0 libuvch264src || echo 'Plugin inspection failed - checking build output:' && ls -la buildDir/src/*.so"]

