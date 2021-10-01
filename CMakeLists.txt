cmake_minimum_required(VERSION 3.20)
project(SMI)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost COMPONENTS json log REQUIRED)
find_package(AWSSDK COMPONENTS s3)

add_library(SMI SHARED src/Communicator.cpp src/utils/Configuration.cpp src/comm/Channel.cpp src/comm/CentralChannel.cpp include/comm/CentralChannel.h src/comm/S3.cpp include/comm/S3.h)

target_include_directories(SMI PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(SMI ${Boost_Libraries} ${AWSSDK_LINK_LIBRARIES} Boost::log)

add_executable(client example/client.cpp)

target_link_libraries(client SMI)