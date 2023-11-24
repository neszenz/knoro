cmake_minimum_required(VERSION 3.27)
project(knoro CXX)

add_subdirectory("vendor/fmt")

add_executable(${CMAKE_PROJECT_NAME} main.cpp util.cpp)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE "--std=c++2b")
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE fmt::fmt)
