
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(TdExample VERSION 1.0 LANGUAGES CXX)

add_subdirectory(td)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE Td::TdStatic)
set_property(TARGET main PROPERTY CXX_STANDARD 14)
