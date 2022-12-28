cmake_minimum_required(VERSION 3.20)
project(ForwardSubstitution)
find_package(TBB REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(ForwardSubstitution main.cpp)
target_link_libraries(ForwardSubstitution TBB::tbb)