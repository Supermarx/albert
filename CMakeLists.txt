cmake_minimum_required(VERSION 2.8)

project(albert)

include_directories("${PROJECT_BINARY_DIR}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/modules")

file(GLOB_RECURSE albert_HEADERS src/*.hpp)
file(GLOB_RECURSE albert_SOURCES src/*.cpp)

add_executable(albert
	${albert_SOURCES}
	${albert_HEADERS}
)

add_definitions("-Wall -Wextra -Weffc++ -std=c++0x -pedantic -g3 -O3")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      "${PROJECT_SOURCE_DIR}/cmake")

find_package(CURL REQUIRED)
find_package(arabica REQUIRED)
find_package(Boost COMPONENTS system program_options regex REQUIRED)

include_directories(SYSTEM
                    ${CURL_INCLUDE_DIRS}
                    ${arabica_INCLUDE_DIRS}
                    ${Boost_INCLUDE_DIRS}
                    ${SuperMarx_INCLUDE_DIRS})
                    
target_link_libraries(albert
                      ${CURL_LIBRARIES}
                      ${arabica_LIBRARIES}
                      ${Boost_LIBRARIES})
