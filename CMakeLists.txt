cmake_minimum_required (VERSION 2.6)
project (MotionDetection)
find_package( OpenCV )
include_directories( ${OpenCV_INCLUDE_DIRS} )
add_executable(motdet motdet.cpp)
target_link_libraries( motdet ${OpenCV_LIBS} )

