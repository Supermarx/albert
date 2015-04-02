project(albert)

find_package(arabica REQUIRED)
find_package(Boost COMPONENTS system program_options regex locale date_time REQUIRED)

include_directories(SYSTEM
                    ${arabica_INCLUDE_DIRS}
                    ${Boost_INCLUDE_DIRS}
                    ${SuperMarx_INCLUDE_DIRS})
                    
add_library(albertcore scraper.cpp util/html_parser.cpp)
target_link_libraries(albertcore
					  supermarx-downloader
                      ${arabica_LIBRARIES}
                      ${Boost_LIBRARIES}) 

add_executable(albert main.cpp)
target_link_libraries(albert
					  albertcore
					  supermarx-api)

include_directories(${Albert_INCLUDE_DIRS})
