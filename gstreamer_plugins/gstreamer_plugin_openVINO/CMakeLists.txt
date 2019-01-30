pkg_check_modules(GLIB2      REQUIRED  glib-2.0)
pkg_check_modules(GSTREAMER  REQUIRED  gstreamer-1.0)
pkg_check_modules(OpenCV     REQUIRED  opencv)
pkg_check_modules(EIGEN3     REQUIRED  eigen3)

find_package(InferenceEngine REQUIRED)

include_directories(${GLIB2_INCLUDE_DIRS})
include_directories(${GSTREAMER_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(gst)
include_directories(gst-libs)
include_directories(gst-libs/safestringlib/include)
include_directories(gst-libs/interface)
include_directories(${InferenceEngine_INCLUDE_DIRS})

if(UNIX)
    include_directories(${LIBVA_INCLUDE_DIRS})
endif()

if(DEFINED ENV{INTELOPENCLSDK})
    include_directories($ENV{INTELOPENCLSDK}/include)
    link_directories($ENV{INTELOPENCLSDK}/lib64)
endif()

if (DEBUG)
    set(CMAKE_BUILD_TYPE debug)
else()
    set(CMAKE_BUILD_TYPE release)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong -shared -fPIE -fPIC -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-no-as-needed")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -D_FORTIFY_SOURCE=2 -O2")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -ggdb -O0")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -D_FORTIFY_SOURCE=2 -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -ggdb -O0")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -z noexecstack -z relro -z now")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-whole-archive")

aux_source_directory(gst-libs/algo DIR_SRCS)
aux_source_directory(gst-libs/ocl DIR_SRCS)
aux_source_directory(gst-libs/ipcclient DIR_SRCS)
aux_source_directory(gst DIR_SRCS)

link_directories(gst-libs/safestringlib)
link_directories(/usr/lib)

add_library (gstcvdlfilter SHARED ${DIR_SRCS})
target_link_libraries(gstcvdlfilter
    safestring
	OpenCL
    ${OpenCV_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    ${GLIB2_LIBRARIES}
	${InferenceEngine_LIBRARIES}
   	dlib
	pthread
	)

add_subdirectory(customer)

install(TARGETS gstcvdlfilter DESTINATION gstreamer-1.0 COMPONENT libraries)
install(DIRECTORY gst-libs/ocl/kernels gst-libs/resources/ DESTINATION libgstcvdl)
install(FILES gst-libs/algo/algoregister.h gst-libs/algo/exinferdata.h gst-libs/algo/exinferenceparser.h
        DESTINATION libgstcvdl) 
