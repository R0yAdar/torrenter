from conan import ConanFile

class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"

    def layout(self):
        self.folders.generators = "conan"

    def requirements(self):
        self.requires("fmt/11.0.2")
        self.requires("openssl/3.3.2")
        self.requires("boost/1.86.0")

    def build_requirements(self):
        self.test_requires("catch2/3.7.0")
