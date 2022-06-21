
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/")
add_executable(nvm_server 
"${CMAKE_CURRENT_SOURCE_DIR}/server.cc"
"${CMAKE_CURRENT_SOURCE_DIR}/rdma_common.cc"
)
add_executable(nvm_clent 
"${CMAKE_CURRENT_SOURCE_DIR}/client.cc"
"${CMAKE_CURRENT_SOURCE_DIR}/rdma_common.cc"
)
target_link_libraries(nvm_server  ibverbs sherman)
target_link_libraries(nvm_clent  ibverbs sherman)

