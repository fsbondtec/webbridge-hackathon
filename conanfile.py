from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMakeToolchain


class UsgHostRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def generate(self):
        # Configure CMakeToolchain for Ninja Multi-Config
        tc = CMakeToolchain(self)
        tc.generator = "Ninja Multi-Config"
        tc.generate()

    def requirements(self):
        self.requires("fmt/11.0.2")
        self.requires("argparse/3.1")
        self.requires("nlohmann_json/3.11.3")
        self.requires("cpp-httplib/0.18.1")
        self.requires("portable-file-dialogs/0.1.0")
        self.requires("magic_enum/0.9.7")

    def build_requirements(self):
        pass

    def layout(self):
        cmake_layout(self)
