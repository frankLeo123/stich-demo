cmake_minimum_required(VERSION 2.8)
project(sv_sample)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -O3 -fopenmp")

execute_process( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )

find_package(OpenCV REQUIRED)
find_package(Gflags REQUIRED)

include_directories(usr/local/include/)

add_executable(testLoadStitch testLoadStitch.cpp)
target_link_libraries(testLoadStitch ${OpenCV_LIBS} gflags)


