cmake_minimum_required(VERSION 3.3)

project(hddl_s)

include(FindPkgConfig)

if(UNIX)
  set(CMAKE_INSTALL_PREFIX "/usr/lib/x86_64-linux-gnu/")
endif()

### third part dependencies

# OpenCV
#find_package (OpenCV REQUIRED)
pkg_check_modules(OpenCV    REQUIRED  opencv)

# eigen3
configure_file(thirdpart/eigen3CMakeLists.txt.in deps/eigen3/CMakeLists.txt)
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/deps/eigen3" )
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/deps/eigen3" )
set(EIGEN3_DIR  "${CMAKE_BINARY_DIR}/eigen3")

# dlib
configure_file(thirdpart/dlibCMakeLists.txt.in deps/dlib/CMakeLists.txt)
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/deps/dlib" )
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/deps/dlib" )
set(DLIB_DIR  "${CMAKE_BINARY_DIR}/dlib")

add_subdirectory(gstreamer_plugins)
