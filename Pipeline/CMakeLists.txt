cmake_minimum_required(VERSION 3.20)
project(Pipeline)
find_package(TBB REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(Pipeline main.cpp)
target_link_libraries(Pipeline TBB::tbb)
