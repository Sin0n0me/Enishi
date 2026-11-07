# syntax=docker/dockerfile:1
FROM ghcr.io/astral-sh/uv:0.9.9 AS uv
FROM ubuntu:24.04

# DXC and the existing Vulkan SDK distribution are Linux x86_64 binaries.
RUN test "$(dpkg --print-architecture)" = amd64
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    ca-certificates curl git cmake ninja-build clang-19 gcc-14 g++-14 \
    pkg-config xz-utils unzip libicu-dev \
    libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev \
    libxfixes-dev libxss-dev libxtst-dev libgl1-mesa-dev libegl1-mesa-dev \
    libasound2-dev libpulse-dev mesa-vulkan-drivers spirv-tools \
    xvfb xauth x11-utils openbox xcompmgr \
    && rm -rf /var/lib/apt/lists/*

COPY --from=uv /uv /uvx /usr/local/bin/
ENV CARGO_HOME=/opt/cargo RUSTUP_HOME=/opt/rustup
ENV PATH="/opt/cargo/bin:/opt/dxc/bin:/opt/python-tools/.venv/bin:${PATH}"
COPY src/engine/script/rust-toolchain.toml /opt/enishi/rust-toolchain.toml
RUN sed -i 's/\r$//' /opt/enishi/rust-toolchain.toml \
    && curl --fail --location --retry 3 https://sh.rustup.rs -o /tmp/rustup.sh \
    && channel=$(sed -n 's/^channel = "\([^"]*\)"/\1/p' /opt/enishi/rust-toolchain.toml) \
    && test -n "$channel" \
    && sh /tmp/rustup.sh -y --profile minimal --default-toolchain "$channel" --no-modify-path \
    && rm /tmp/rustup.sh

COPY pyproject.toml uv.lock .python-version /opt/python-tools/
RUN cd /opt/python-tools && uv sync --locked

RUN mkdir -p /opt/dxc \
    && curl --fail --location --retry 3 \
       https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2505.1/linux_dxc_2025_07_14.x86_64.tar.gz \
       -o /tmp/dxc.tar.gz \
    && tar -xzf /tmp/dxc.tar.gz -C /opt/dxc && rm /tmp/dxc.tar.gz

# Reuse the CI SDK version, checksum, and SPIRV-Reflect discovery.
COPY .github/scripts/setup_linux_vulkan.sh /opt/enishi/setup_linux_vulkan.sh
RUN sed -i 's/\r$//' /opt/enishi/setup_linux_vulkan.sh \
    && RUNNER_TEMP=/opt GITHUB_ENV=/opt/enishi/vulkan.env GITHUB_PATH=/opt/enishi/vulkan.path \
       bash /opt/enishi/setup_linux_vulkan.sh \
    && rm /opt/enishi-vulkan.tar.xz

COPY docker/linux/build.sh /opt/enishi/build.sh
RUN sed -i 's/\r$//' /opt/enishi/build.sh
ENV LIBGL_ALWAYS_SOFTWARE=1 SDL_VIDEO_DRIVER=x11
WORKDIR /workspace
ENTRYPOINT ["bash", "/opt/enishi/build.sh"]
CMD ["opengl", "Debug", "build", "clang"]
