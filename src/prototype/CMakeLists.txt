project(albert-prototype)

find_package(CURL REQUIRED)
find_package(arabica REQUIRED)
find_package(Boost COMPONENTS system program_options regex locale REQUIRED)

include_directories(SYSTEM
					${Albert_INCLUDE_DIRS}
					${Karl_INCLUDE_DIRS})
                    
add_executable(albert-prototype main.cpp)
target_link_libraries(albert-prototype libkarl libalbert)
