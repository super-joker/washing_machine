# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "D:/esp/v6.1/esp-idf/components/bootloader/subproject")
  file(MAKE_DIRECTORY "D:/esp/v6.1/esp-idf/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "E:/WorkSpace/Code/01_ESP32/02_washing_machine_v01_20261021/build/bootloader"
  "E:/WorkSpace/Code/01_ESP32/02_washing_machine_v01_20261021/build/bootloader-prefix"
  "E:/WorkSpace/Code/01_ESP32/02_washing_machine_v01_20261021/build/bootloader-prefix/tmp"
  "E:/WorkSpace/Code/01_ESP32/02_washing_machine_v01_20261021/build/bootloader-prefix/src/bootloader-stamp"
  "E:/WorkSpace/Code/01_ESP32/02_washing_machine_v01_20261021/build/bootloader-prefix/src"
  "E:/WorkSpace/Code/01_ESP32/02_washing_machine_v01_20261021/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "E:/WorkSpace/Code/01_ESP32/02_washing_machine_v01_20261021/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "E:/WorkSpace/Code/01_ESP32/02_washing_machine_v01_20261021/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
