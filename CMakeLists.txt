cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/wubwubcmake/modules")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/modules")
message(${CMAKE_MODULE_PATH})

include("contrib/wubwubcmake/enable_cpp11.cmake")
include("contrib/wubwubcmake/warning_settings.cmake")
add_sane_warning_flags()

list(APPEND Albert_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/src/")
set(Albert_INCLUDE_DIRS ${Albert_INCLUDE_DIRS} PARENT_SCOPE)

add_subdirectory("src/albert")
