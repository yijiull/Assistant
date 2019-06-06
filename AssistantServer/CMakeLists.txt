cmake_minimum_required(VERSION 3.0.0)

SET(CMAKE_CXX_COMPILER "g++") 
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

project(Server VERSION 0.1.0)


add_executable(mainServer ./src/mainServer.cpp ./src/util.cpp ./src/Worker.cpp 
./src/msql.cpp ./src/mcrypto.cpp)

add_executable(FileServer ./src/FileServer.cpp ./src/util.cpp ./src/mcrypto.cpp ./src/msql.cpp)

target_link_libraries(mainServer -lcrypto -lmysqlclient -lglog)
target_link_libraries(FileServer -lcrypto -lmysqlclient -lglog)
