#!/usr/bin/env bash
case $1 in
    "" | "windows")
        base_dir="/run/media/qwix456/mydisk/geode/cross-platform"
        splat_dir="$base_dir/splat"
        toolchain="$base_dir/clang-msvc-sdk/clang-cl-msvc.cmake"

        export WINSDK_VER="10.0.26100"
        export MSVC_BASE="$splat_dir/crt"
        export WINSDK_BASE="$splat_dir/sdk"
        export HOST_ARCH="x86_64"

        export CFLAGS="-MD"
        export CXXFLAGS="-MD"

        geode build -p windows -- \
                    -GNinja \
                    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
                    -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
                    -DMSVC_BASE="$MSVC_BASE" \
                    -DCMAKE_POLICY_VERSION_MINIMUM="3.5" \
                    -DWINSDK_BASE="$WINSDK_BASE" \
                    -DWINSDK_VER="$WINSDK_VER" \
                    -DHOST_ARCH="x86_64" \
                    -DCMAKE_BUILD_TYPE=Release \
                    -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreadedDLL" ;;
esac
