pkg_check_modules(GLIB2      REQUIRED  glib-2.0)
pkg_check_modules(GSTREAMER  REQUIRED  gstreamer-1.0)
pkg_check_modules(JSON-C     REQUIRED  json-c)

include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(${GSTREAMER_INCLUDE_DIRS})

if (DEBUG)
    set(CMAKE_BUILD_TYPE debug)
else()
    set(CMAKE_BUILD_TYPE release)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong -fPIE -fPIC -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-no-as-needed")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -D_FORTIFY_SOURCE=2 -O2")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -ggdb -O0")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -D_FORTIFY_SOURCE=2 -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -ggdb -O0")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -z noexecstack -z relro -z now -pie")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath-link -lstdc++ -std=c++11 -pthread")

link_directories(${PROJECT_ROOT_PATH}/gstreamer_plugins/gstreamer_plugin_openVINO/gst-libs/safestringlib)

add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(test)

