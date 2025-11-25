# DeepStream 7.1 Development Container for Laptop/Desktop
FROM nvcr.io/nvidia/deepstream:7.1-gc-triton-devel

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Set up workspace
WORKDIR /workspace

# Install essential development tools and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    curl \
    gdb \
    git \
    gstreamer1.0-alsa \
    gstreamer1.0-gl \
    gstreamer1.0-gtk3 \
    gstreamer1.0-libav \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-pulseaudio \
    gstreamer1.0-qt5 \
    gstreamer1.0-tools \
    gstreamer1.0-x \
    libglib2.0-dev \
    libgmock-dev \
    libgstreamer-plugins-bad1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer1.0-dev \
    libgstrtspserver-1.0-0 \
    libgstrtspserver-1.0-dev \
    libgtest-dev \
    libjansson-dev \
    libjson-glib-dev \
    libopencv-dev \
    librdkafka-dev \
    libssl-dev \
    ninja-build \
    python-is-python3 \
    python3-dev \
    python3-opencv \
    python3-pip \
    valgrind \
    vim \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Install Python packages for DeepStream development
RUN pip3 install --no-cache-dir \
    numpy \
    opencv-python \
    pycairo \
    jupyterlab \
    ipython \
    pytest \
    black \
    pylint \
    autopep8

# Install additional useful tools for development
RUN apt-get update && apt-get install -y \
    htop \
    tmux \
    tree \
    jq \
    sudo \
    software-properties-common \
    && rm -rf /var/lib/apt/lists/*

# Set up PyGObject for Python bindings
RUN apt-get update && apt-get install -y \
    python3-gi \
    python3-gi-cairo \
    gir1.2-gstreamer-1.0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp
# Install spdlog from source v1.13.0
RUN git clone --depth 1 --branch v1.13.0 https://github.com/gabime/spdlog.git && \
    cd spdlog && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DSPDLOG_BUILD_SHARED=ON \
             -DCMAKE_POSITION_INDEPENDENT_CODE=ON && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    rm -rf /tmp/spdlog
# Install expected-lite from source v0.10.0
RUN git clone --depth 1 --branch v0.10.0 https://github.com/nonstd-lite/expected-lite.git && \
    cd expected-lite && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    rm -rf /tmp/expected-lite
# Install tracy from source v0.13.0
RUN git clone --depth 1 --branch v0.13.0 https://github.com/wolfpld/tracy.git && \
    cd tracy && \
    mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    rm -rf /tmp/tracy

# Create a non-root user for development
ARG USERNAME=developer
ARG USER_UID=1000
ARG USER_GID=$USER_UID

COPY scripts/setup_user.sh /tmp/setup_user.sh
# Run the setup script
RUN bash /tmp/setup_user.sh "$USERNAME" "$USER_UID" "$USER_GID" && rm /tmp/setup_user.sh

# Set up DeepStream Python bindings path
ENV PYTHONPATH=/opt/nvidia/deepstream/deepstream/lib:$PYTHONPATH
ENV LD_LIBRARY_PATH=/opt/nvidia/deepstream/deepstream/lib:$LD_LIBRARY_PATH
ENV GST_PLUGIN_PATH=/opt/nvidia/deepstream/deepstream/lib/gst-plugins:$GST_PLUGIN_PATH

# Copy DeepStream samples to workspace for easy access
RUN cp -r /opt/nvidia/deepstream/deepstream/sources /workspace/deepstream-samples && \
    chown -R $USERNAME:$USERNAME /workspace/deepstream-samples

# Set default user
USER $USERNAME

# Set working directory
WORKDIR /workspace

# Keep container running
CMD ["/bin/bash"]
