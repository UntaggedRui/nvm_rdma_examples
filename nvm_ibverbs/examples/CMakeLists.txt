add_executable(rdmaread_client  "${CMAKE_CURRENT_SOURCE_DIR}/RdmaRead/client.cc")
add_executable(rdmaread_server  "${CMAKE_CURRENT_SOURCE_DIR}/RdmaRead/server.cc")

add_executable(rdmawrite_client  "${CMAKE_CURRENT_SOURCE_DIR}/RdmaWrite/client.cc")
add_executable(rdmawrite_server  "${CMAKE_CURRENT_SOURCE_DIR}/RdmaWrite/server.cc")

add_executable(send_recv_client  "${CMAKE_CURRENT_SOURCE_DIR}/send_recv/rdma_recv.cc")
add_executable(send_recv_server  "${CMAKE_CURRENT_SOURCE_DIR}/send_recv/rdma_send.cc")

set(exps
rdmaread_client rdmaread_server
rdmawrite_client rdmawrite_server
send_recv_client send_recv_server)


foreach(e ${exps})
 target_link_libraries(${e} rdmacommon ibverbs sherman)
endforeach(e)