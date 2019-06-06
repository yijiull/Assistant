cmake_minimum_required(VERSION 3.0.0)
project(mainClient VERSION 0.1.0)

SET(CMAKE_CXX_COMPILER "g++") 
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -pthread")

add_library(myclient SHARED ./src/dll.cpp ./src/util.cpp ./src/FileManager.cpp ./src/mcrypto.cpp)
set_target_properties(myclient PROPERTIES PUBLIC_HEADER dll.h)
target_include_directories(myclient PRIVATE .)

add_executable(mainClient ./src/mainClient.cpp ./src/dll.cpp ./src/util.cpp ./src/FileManager.cpp ./src/mcrypto.cpp)

target_link_libraries(mainClient -lcrypto -lpthread -lglog)
