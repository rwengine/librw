# -*- coding: utf-8 -*-

from conans import ConanFile, CMake, tools
from conans.errors import ConanInvalidConfiguration
import os


class LibrwConan(ConanFile):
    name = "librw"
    version = "master"
    license = "MIT"
    author = "aap <angelo.papenhoff@gmail.com"
    url = "https://www.github.com/rwengine/librw"
    description = "A (partial) re-implementation of RenderWare Graphics"
    topics = ("RenderWare", "Engine", "graphics", )
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = (
        "*.h",
        "CMakeLists.txt",
        "CMakeCPack.cmake",
        "librw-config.cmake.in",
        "cmake",
        "src/**",
        "skeleton/**",
        "tools/**",
        "LICENSE",
    )
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "platform": ["null", "gl3", "d3d", "ps2",]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "platform": "null",
    }
    generators = "cmake"

    def config_options(self):
        if self.settings.os != "Windows" and self.options.platform == "d3d":
            raise ConanInvalidConfiguration("Current os does not support d3d")

    def build_requirements(self):
        if self.options.platform == "ps2":
            self.build_requires("ps2_toolchain_installer@rwengine/stable")

    def requirements(self):
        if self.options.platform == "gl3":
            self.requires("sdl2/2.0.9@bincrafters/stable")
            self.requires("glew/2.1.0@bincrafters/stable")

    @property
    def _librw_platform(self):
        return str(self.options.platform).upper()

    def build(self):
        cmake = CMake(self)
        cmake.definitions["LIBRW_PLATFORM"] = self._librw_platform
        cmake.definitions["LIBRW_INSTALL"] = "ON"
        cmake.definitions["LIBRW_TOOLS"] = "ON"
        cmake.configure()
        cmake.build()

    def package(self):
        with tools.chdir(self.build_folder):
            cmake = CMake(self)
            cmake.install()
        self.copy("LICENSE", src=self.source_folder, dst="licenses")

    def package_info(self):
        self.cpp_info.includedirs = [
            os.path.join(self.package_folder, "include"),
            os.path.join(self.package_folder, "include", "librw"),
        ]
        self.cpp_info.libs = ["rw", "rw_skeleton", ]
        self.user_info.LIBRW_PLATFORM = self._librw_platform
