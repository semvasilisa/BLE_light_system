# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "D:/projects/ble_light_project")
  file(MAKE_DIRECTORY "D:/projects/ble_light_project")
endif()
file(MAKE_DIRECTORY
  "D:/projects/ble_light_project/build/ble_light_project"
  "D:/projects/ble_light_project/build/_sysbuild/sysbuild/images/ble_light_project-prefix"
  "D:/projects/ble_light_project/build/_sysbuild/sysbuild/images/ble_light_project-prefix/tmp"
  "D:/projects/ble_light_project/build/_sysbuild/sysbuild/images/ble_light_project-prefix/src/ble_light_project-stamp"
  "D:/projects/ble_light_project/build/_sysbuild/sysbuild/images/ble_light_project-prefix/src"
  "D:/projects/ble_light_project/build/_sysbuild/sysbuild/images/ble_light_project-prefix/src/ble_light_project-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/projects/ble_light_project/build/_sysbuild/sysbuild/images/ble_light_project-prefix/src/ble_light_project-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/projects/ble_light_project/build/_sysbuild/sysbuild/images/ble_light_project-prefix/src/ble_light_project-stamp${cfgdir}") # cfgdir has leading slash
endif()
