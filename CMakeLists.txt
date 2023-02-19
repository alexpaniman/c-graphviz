cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(simple-graphviz-test VERSION 1.0)

add_subdirectory(lib)

add_executable(main main.cpp)
target_link_libraries(main graphviz)
