
pkg_check_modules(GLIB2      REQUIRED  glib-2.0)
pkg_check_modules(GSTREAMER  REQUIRED  gstreamer-1.0)

include_directories(..)
include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(${GSTREAMER_INCLUDE_DIRS})

aux_source_directory(. SRC_LIST_C)

link_directories(/usr/lib)
add_library (ws_client STATIC ${SRC_LIST_C})
target_link_libraries(ipc_client uWS)
