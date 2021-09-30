cmake_minimum_required(VERSION 3.20)
project(SMI)

set(CMAKE_CXX_STANDARD 17)

add_library(SMI SHARED src/Communicator.cpp)
