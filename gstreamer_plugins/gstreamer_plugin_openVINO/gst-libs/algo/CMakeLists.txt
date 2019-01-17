
#aux_source_directory(./ SRC_LIST)


pkg_check_modules(GLIB2      REQUIRED  glib-2.0)
pkg_check_modules(GSTREAMER  REQUIRED  gstreamer-1.0)
pkg_check_modules(LIBVA      REQUIRED  libva)

find_package(InferenceEngine REQUIRED)

if(DEFINED ENV{INTELOPENCLSDK})
  include_directories($ENV{INTELOPENCLSDK}/include)
endif()

include_directories(./src)
include_directories(..)
include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(${GSTREAMER_INCLUDE_DIRS})
include_directories(${LIBVA_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${InferenceEngine_INCLUDE_DIRS})

include_directories(${EIGEN3_DIR})
include_directories(${DLIB_DIR})

aux_source_directory(. SRC_LIST_C)

add_subdirectory(${DLIB_DIR}/dlib  dlib_build)	
add_library (cvdl_algo SHARED ${SRC_LIST_C})
target_link_libraries(cvdl_algo ${InferenceEngine_LIBRARIES} dlib::dlib)

install(TARGETS cvdl_algo DESTINATION lib COMPONENT libraries)
#target_link_libraries(cvdl_algo ${OpenCV_LIBS} ${InferenceEngine_LIBRARIES} dlib::dlib)
#set_property(TARGET cvdl_algo PROPERTY CXX_STANDARD 11)
