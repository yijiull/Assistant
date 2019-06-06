cmake_minimum_required(VERSION 3.0.0)
project(mainClient VERSION 0.1.0)

SET(CMAKE_CXX_COMPILER "g++") 
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -pthread")

add_executable(mainClient ./mainClient.cpp)

target_link_libraries(mainClient myclient)
