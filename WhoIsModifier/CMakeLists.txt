cmake_minimum_required(VERSION 3.25)
project(WhoIsModifier)

set(CMAKE_CXX_STANDARD 20)
include_directories(/usr/include/mongocxx/v_noabi)
include_directories(/usr/include/bsoncxx/v_noabi)
add_executable(WhoIsModifier main.cpp DbModifyingOps.cpp DbModifyingOps.h DbRenewOp.cpp DbRenewOp.h DbDeleteOp.cpp DbDeleteOp.h)
target_link_libraries(WhoIsModifier bsoncxx mongocxx)