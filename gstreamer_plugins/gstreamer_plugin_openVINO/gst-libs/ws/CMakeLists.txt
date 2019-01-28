
#aux_source_directory(./ SRC_LIST)


pkg_check_modules(GLIB2      REQUIRED  glib-2.0)
pkg_check_modules(GSTREAMER  REQUIRED  gstreamer-1.0)

include_directories(./src)
include_directories(..)
include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(${GSTREAMER_INCLUDE_DIRS})

aux_source_directory(. SRC_LIST_C)

add_library (ws_client STATIC ${SRC_LIST_C})
#install(TARGETS ws_client DESTINATION lib COMPONENT libraries)
#target_link_libraries(ws_client dlib::dlib)
set_property(TARGET ws_client PROPERTY CXX_STANDARD 11)
