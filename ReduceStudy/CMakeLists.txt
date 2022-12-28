cmake_minimum_required(VERSION 3.20)
project(Algorithms)
find_package(TBB REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(Algorithms main.cpp)
target_link_libraries(Algorithms TBB::tbb)