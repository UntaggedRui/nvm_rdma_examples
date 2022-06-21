include_directories(
        .
)
add_executable(nclient "${CMAKE_SOURCE_DIR}/nvm_write/client.cc" )
add_executable(nserver "${CMAKE_SOURCE_DIR}/nvm_write/server.cc" )
set(exps
nclient nserver)
foreach(e ${exps})
 target_link_libraries(${e} pthread ibverbs)
endforeach(e)

