add_subdirectory(gstreamer_plugin_openVINO)

include(ExternalProject)
ExternalProject_Add(msdk-gst 
                    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/gstreamer-media-SDK"
#                    INSTALL_DIR "${CMAKE_INSTALL_PREFIX}"
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libgstmfx.so" DESTINATION gstreamer-1.0 COMPONENT libraries)