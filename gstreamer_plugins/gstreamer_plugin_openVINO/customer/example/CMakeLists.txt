
include_directories(.)

aux_source_directory(. CUSTOMER_DIR_SRCS)
link_directories(/usr/lib)

add_library (algoexample SHARED ${CUSTOMER_DIR_SRCS})
target_link_libraries(algoexample m)
