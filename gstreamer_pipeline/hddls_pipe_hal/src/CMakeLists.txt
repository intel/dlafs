
aux_source_directory(. DIR_SRCS)

include_directories(${PROJECT_ROOT_PATH}/gstreamer_plugins/gstreamer_plugin_openVINO/gst-libs/ws)

add_executable (hddlspipes ${DIR_SRCS})
target_link_libraries(hddlspipes
	gstcvdlfilter
	${GSTREAMER_LIBRARIES}
	${GLIB2_LIBRARIES}
	pthread
    json-c
)

install(TARGETS hddlspipes DESTINATION ../../local/bin)
