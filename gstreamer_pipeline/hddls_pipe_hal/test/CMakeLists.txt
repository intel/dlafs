
aux_source_directory(. DIR_SRCS)

include_directories(/usr/include)
include_directories(${PROJECT_ROOT_PATH}/gstreamer_plugins/gstreamer_plugin_openVINO/gst-libs/ws)
#link_directories(${PROJECT_ROOT_PATH}/build/gstreamer_plugins/gstreamer_plugin_openVINO)

add_executable (hddlspipestest ${DIR_SRCS})
target_link_libraries(hddlspipestest
    gstcvdlfilter
	${GSTREAMER_LIBRARIES}
	${GLIB2_LIBRARIES}
    pthread
    json-c
)

install(TARGETS hddlspipestest DESTINATION ../../local/bin)
