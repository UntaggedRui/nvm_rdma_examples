
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(rdmacommon STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/rdma_common.cc")
set_target_properties(sherman PROPERTIES LINKER_LANGUAGE CXX)
add_subdirectory(examples)


