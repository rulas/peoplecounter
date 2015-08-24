cmake_minimum_required (VERSION 2.6)
project (PeopleCounter)
find_package( OpenCV )
include_directories( ${OpenCV_INCLUDE_DIRS} )
add_executable(pcounter pcounter.cpp)
target_link_libraries( pcounter ${OpenCV_LIBS} )

