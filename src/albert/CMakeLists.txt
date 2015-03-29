project(albert)

add_executable(albert
	main.cpp
	scraper.cpp
	util/html_parser.cpp
	util/downloader.cpp)

find_package(CURL REQUIRED)
find_package(arabica REQUIRED)
find_package(Boost COMPONENTS system program_options regex locale REQUIRED)

include_directories(SYSTEM
                    ${CURL_INCLUDE_DIRS}
                    ${arabica_INCLUDE_DIRS}
                    ${Boost_INCLUDE_DIRS}
                    ${SuperMarx_INCLUDE_DIRS})
                    
target_link_libraries(albert
                      ${CURL_LIBRARIES}
                      ${arabica_LIBRARIES}
                      ${Boost_LIBRARIES})
