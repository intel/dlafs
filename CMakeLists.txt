cmake_minimum_required(VERSION 3.3)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(hddl_s)

include(FindPkgConfig)

SET(PROJECT_ROOT_PATH "${hddl_s_SOURCE_DIR}")
if(UNIX)
  set(CMAKE_INSTALL_PREFIX "/usr/lib/x86_64-linux-gnu/")
endif()

### third part dependencies


set(OPENCV_VERSION "4.0.0")

# OpenCV
#find_package (OpenCV REQUIRED)
if(USE_OPENCV_3_4_x)
	#OpenCV 3.4.1
	pkg_check_modules(OpenCV    REQUIRED  opencv)
else()
	#OpenCV 4.0.0
	pkg_check_modules(OpenCV    REQUIRED  opencv4)
endif()

# eigen3
#configure_file(thirdpart/eigen3CMakeLists.txt.in deps/eigen3/CMakeLists.txt)
#execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
#    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/deps/eigen3" )
#execute_process(COMMAND "${CMAKE_COMMAND}" --build .
#    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/deps/eigen3" )
#set(EIGEN3_DIR  "${CMAKE_BINARY_DIR}/eigen3")
pkg_check_modules(EIGEN3 REQUIRED eigen3)

# dlib
#configure_file(thirdpart/dlibCMakeLists.txt.in deps/dlib/CMakeLists.txt)
#execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
#    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/deps/dlib" )
#execute_process(COMMAND "${CMAKE_COMMAND}" --build .
#    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/deps/dlib" )
#set(DLIB_DIR  "${CMAKE_BINARY_DIR}/dlib")
#pkg_check_modules(DLIB REQUIRED dlib)

add_subdirectory(gstreamer_plugins)
add_subdirectory(gstreamer_pipeline)
