include_directories("$(CMAKE_CURRENT_SOURCE_DIR)")
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}" nicmem_src)
add_executable(nicmem 
"${nicmem_src}"
)
target_link_libraries(nicmem  ibverbs sherman)

