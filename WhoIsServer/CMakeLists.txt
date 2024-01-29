cmake_minimum_required(VERSION 3.25)
project(WhoIsServer)

set(CMAKE_CXX_STANDARD 17)
include_directories(/usr/include/mongocxx/v_noabi)
include_directories(/usr/include/bsoncxx/v_noabi)
add_executable(WhoIsServer main.cpp)
target_link_libraries(WhoIsServer bsoncxx mongocxx)