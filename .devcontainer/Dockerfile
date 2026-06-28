# DeepStream 7.1 Development Container for Laptop/Desktop
FROM nvcr.io/nvidia/deepstream:9.0-samples-multiarch

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ninja-build \
    libgtest-dev \
    libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

# Set up workspace
WORKDIR /workspace

WORKDIR /tmp

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
