cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(PifTDPony VERSION 1.0 LANGUAGES CXX)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

find_package(Td 1.1.1 REQUIRED)

add_library(pif SHARED main.cpp)
target_link_libraries(pif PRIVATE Td::TdJson phpcpp)
set_property(TARGET pif PROPERTY CXX_STANDARD 14)

add_custom_command(TARGET pif POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                       ${CMAKE_CURRENT_SOURCE_DIR}/pif.ini
                       $<TARGET_FILE_DIR:pif>
                   )

execute_process(COMMAND bash "-c" "php --ini | sed '/Scan for additional .ini files in: /!d;s/Scan for additional .ini files in: //'" OUTPUT_VARIABLE INI_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND php-config --extension-dir OUTPUT_VARIABLE EXTENSION_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)


install(TARGETS pif DESTINATION ${EXTENSION_DIR})
install(FILES pif.ini DESTINATION ${INI_DIR})

