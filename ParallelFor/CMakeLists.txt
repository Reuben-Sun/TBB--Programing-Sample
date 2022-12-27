cmake_minimum_required(VERSION 3.20)
project(TimeStudy)

find_package(TBB REQUIRED)

set(CMAKE_CXX_STANDARD 17)

add_executable(TimeStudy main.cpp ImageLib.h)

target_link_libraries(TimeStudy TBB::tbb)
