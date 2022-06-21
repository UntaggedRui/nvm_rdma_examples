include_directories("${CMAKE_SOURCE_DIR}/thirdparty/Sherman")
set(SHERMAN_SRC Debug.cc)
add_library(sherman STATIC ${SHERMAN_SRC})
set_target_properties(sherman PROPERTIES LINKER_LANGUAGE CXX)
