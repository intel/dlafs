
aux_source_directory(. DIR_SRCS)

include_directories(${PROJECT_ROOT_PATH}/gstreamer_plugins/gstreamer_plugin_openVINO/gst-libs/algo)

add_executable (registeralgo ${DIR_SRCS})
target_link_libraries(registeralgo
    gstcvdlfilter
	${GSTREAMER_LIBRARIES}
	${GLIB2_LIBRARIES}
)

install(TARGETS registeralgo DESTINATION ../../local/bin)
