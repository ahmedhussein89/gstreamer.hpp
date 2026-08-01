vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ahmedhussein89/gstreamer.hpp
    REF "v${VERSION}"
    # Update on every release: vcpkg_from_github prints the correct value on mismatch.
    SHA512 0
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DGST_BUILD_TUTORIALS=OFF
        -DGST_BUILD_TESTS=OFF
        -DGST_USE_SYSTEM_DEPS=ON
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME gstreamer-hpp CONFIG_PATH lib/cmake/gstreamer-hpp)

# Header-only: nothing lands in the debug tree.
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
