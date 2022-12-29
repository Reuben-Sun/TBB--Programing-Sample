cmake_minimum_required(VERSION 3.20)
project(FlowGraph2)
find_package(TBB REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(FlowGraph2 main.cpp)
target_link_libraries(FlowGraph2 TBB::tbb)
