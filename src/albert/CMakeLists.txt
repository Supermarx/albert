project(albert)

find_package(CURL REQUIRED)
find_package(arabica REQUIRED)
find_package(Boost COMPONENTS system program_options regex locale REQUIRED)

include_directories(SYSTEM
                    ${CURL_INCLUDE_DIRS}
                    ${arabica_INCLUDE_DIRS}
                    ${Boost_INCLUDE_DIRS}
                    ${SuperMarx_INCLUDE_DIRS})
                    
add_library(libalbert scraper.cpp util/html_parser.cpp util/downloader.cpp)
target_link_libraries(libalbert
                      ${CURL_LIBRARIES}
                      ${arabica_LIBRARIES}
                      ${Boost_LIBRARIES})

add_executable(albert main.cpp)
target_link_libraries(albert libalbert)

