
#aux_source_directory(./ SRC_LIST)


pkg_check_modules(GLIB2      REQUIRED  glib-2.0)
pkg_check_modules(GSTREAMER  REQUIRED  gstreamer-1.0)

include_directories(./src)
include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(${GSTREAMER_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(../gst-libs)
include_directories(../gst-libs/safestringlib/include)
#include_directories(C:/gstreamer/1.0/x86_64/include)
#include_directories(C:/gstreamer/1.0/x86_64/include)
#include_directories(C:/Users/lianggao/Documents/test/libuv-1.23.2/include)

#link_directories(C:/gstreamer/1.0/x86_64/lib)
#link_directories(C:/Users/lianggao/Documents/test/libuv-1.23.2/Release/lib)
#link_directories(./thirdpart/uWebSockets/build/x64/Debug)
#link_directories(./build/x64/Debug)
aux_source_directory(. SRC_LIST_C)


add_library (gstcvdlfilter SHARED ${SRC_LIST_C})
target_link_libraries(gstcvdlfilter )
install(TARGETS gstcvdlfilter DESTINATION lib COMPONENT libraries)
#set_property(TARGET HelloWorld PROPERTY CXX_STANDARD 17)
