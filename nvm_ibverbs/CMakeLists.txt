
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_library(rdmacommon STATIC 
"${CMAKE_CURRENT_SOURCE_DIR}/src/rdma_common.cc"
"${CMAKE_CURRENT_SOURCE_DIR}/src/util.cc"
)
set_target_properties(rdmacommon PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(rdmacommon pthread)
add_subdirectory(examples)


