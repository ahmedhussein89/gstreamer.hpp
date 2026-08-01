import os

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.files import copy
from conan.tools.layout import basic_layout


class GstreamerHppConan(ConanFile):
    name = "gstreamer-hpp"
    version = "0.1.0"
    description = "A header-only, modern C++20 wrapper for GStreamer, inspired by vulkan.hpp"
    homepage = "https://github.com/ahmedhussein89/gstreamer.hpp"
    url = homepage
    topics = ("gstreamer", "multimedia", "wrapper", "header-only")

    package_type = "header-library"
    settings = "os", "arch", "compiler", "build_type"
    no_copy_source = True
    exports_sources = "include/*", "LICENSE"

    def requirements(self):
        self.requires("fmt/11.0.2", transitive_headers=True)
        self.requires("expected-lite/0.8.0", transitive_headers=True)
        self.requires("gstreamer/1.24.7", transitive_headers=True, transitive_libs=True)

    def validate(self):
        check_min_cppstd(self, 20)

    def layout(self):
        basic_layout(self, src_folder=".")

    def package_id(self):
        self.info.clear()

    def package(self):
        copy(self, "LICENSE", self.source_folder, os.path.join(self.package_folder, "licenses"))
        copy(self, "*.hpp",
             os.path.join(self.source_folder, "include"),
             os.path.join(self.package_folder, "include"))

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.set_property("cmake_file_name", "gstreamer-hpp")
        self.cpp_info.set_property("cmake_target_name", "gstreamer::hpp")
