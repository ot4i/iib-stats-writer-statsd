from conans import ConanFile, CMake

class StatsWriterStatsd(ConanFile):
  settings = "os", "compiler", "build_type", "arch"
  requires = "Boost/1.60.0@lasote/stable", "gtest/1.8.0@lasote/stable" # comma separated list of requirements
  generators = "cmake"

  def config(self):
    if self.settings.os == "Linux":
      self.options["Boost"].fPIC = True
