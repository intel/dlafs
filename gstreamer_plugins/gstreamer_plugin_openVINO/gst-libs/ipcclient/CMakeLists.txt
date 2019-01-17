
#aux_source_directory(./ SRC_LIST)


pkg_check_modules(GLIB2      REQUIRED  glib-2.0)
pkg_check_modules(GSTREAMER  REQUIRED  gstreamer-1.0)

find_package(InferenceEngine REQUIRED)

include_directories(./src)
include_directories(..)
include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(${GSTREAMER_INCLUDE_DIRS})

aux_source_directory(. SRC_LIST_C)

add_library (ipc_client SHARED ${SRC_LIST_C})
target_link_libraries(ipc_client ${OpenCV_LIBS} ${InferenceEngine_LIBRARIES} dlib::dlib)

install(TARGETS ipc_client DESTINATION lib COMPONENT libraries)
#set_property(TARGET ipc_client PROPERTY CXX_STANDARD 11)
