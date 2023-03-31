add_executable(rdma_rw_bench 
 "${CMAKE_CURRENT_SOURCE_DIR}/readwrite/rdma_bench.cc"
 "${CMAKE_CURRENT_SOURCE_DIR}/readwrite/histogram.cc"
 )
set(exps
rdma_rw_bench)

option(USE_NVM "USE NVM" OFF)
foreach(e ${exps})
 target_link_libraries(${e} rdmacommon ibverbs sherman  gflags)
 if(USE_NVM)
    target_link_libraries(${e} pmem)
 endif()
endforeach(e)