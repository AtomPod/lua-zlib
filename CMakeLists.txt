cmake_minimum_required(VERSION 2.8)

project(zlib)
aux_source_directory("." SRC_CPP_SET)
set(LUA_PATH "x:/lua/lua-5.3.4")
link_directories(${LUA_PATH}/bin)
include_directories(${LUA_PATH}/include)
link_libraries(z lua53)
add_library(${PROJECT_NAME} SHARED ${SRC_CPP_SET})
