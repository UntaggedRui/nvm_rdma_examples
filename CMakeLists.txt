cmake_minimum_required(VERSION 3.3)

project(FORD)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-result -fPIC")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 ")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0  -g")
endif()

include_directories(
        include
        thirdparty
)

add_subdirectory(thirdparty/Sherman)
add_subdirectory(nicmem)
# add_subdirectory(nvm_write)
# add_subdirectory(readwrite)
add_subdirectory(nvm_ibverbs)
