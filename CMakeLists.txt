cmake_minimum_required(VERSION 3.20)
project(SMI)
set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Boost COMPONENTS json log REQUIRED)
find_package(AWSSDK COMPONENTS s3 REQUIRED)
find_package(hiredis REQUIRED)

add_subdirectory(extern/TCPunch/client/)

add_library(SMI SHARED src/Communicator.cpp src/utils/Configuration.cpp src/comm/Channel.cpp src/comm/ClientServer.cpp
        src/comm/S3.cpp src/comm/Redis.cpp src/utils/ChannelPolicy.cpp src/comm/PeerToPeer.cpp src/comm/Direct.cpp)

target_include_directories(SMI PRIVATE ${Boost_INCLUDE_DIRS} ${TCPUNCH_INCLUDE_DIRS})

target_link_libraries(SMI ${Boost_Libraries} Boost::log ${AWSSDK_LINK_LIBRARIES} ${HIREDIS_LIBRARIES} tcpunch)

find_package(OpenMP REQUIRED)

add_executable(client tests/client.cpp)

target_link_libraries(client SMI OpenMP::OpenMP_CXX)

add_subdirectory(tests)