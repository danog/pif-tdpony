
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(PifTDPony VERSION 1.0 LANGUAGES CXX)

add_subdirectory(td)
include_directories(td/td/generate/auto)
include_directories(td/tdutils)

add_library(pif SHARED main.cpp)
target_link_libraries(pif PRIVATE Td::TdJsonStatic phpcpp)
set_property(TARGET pif PROPERTY CXX_STANDARD 14)
