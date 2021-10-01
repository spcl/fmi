cmake_minimum_required(VERSION 3.20)
project(SMI)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost COMPONENTS json REQUIRED)

add_library(SMI SHARED src/Communicator.cpp src/utils/Configuration.cpp src/comm/Channel.cpp)

target_include_directories(SMI PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(SMI ${Boost_Libraries})

add_executable(client example/client.cpp)

target_link_libraries(client SMI)