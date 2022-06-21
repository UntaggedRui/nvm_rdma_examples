add_executable(rclient "${CMAKE_SOURCE_DIR}/readwrite/client.cc")
add_executable(rserver "${CMAKE_SOURCE_DIR}/readwrite/server.cc")
set(exps
rclient rserver)
foreach(e ${exps})
 target_link_libraries(${e} pthread ibverbs)
endforeach(e)

