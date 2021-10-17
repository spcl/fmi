cmake_minimum_required(VERSION 3.10)
project(smi)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/.." libsmi)

find_package(Boost COMPONENTS system python36 REQUIRED)
find_package(Python3 3.6 COMPONENTS Interpreter Development)

set(CMAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(smi MODULE smi_python.cpp PythonCommunicator.cpp)

target_include_directories(smi PRIVATE ${Python3_INCLUDE_DIRS} ${SMI_INCLUDE_DIRS})

target_link_libraries(smi Boost::python36 ${Python3_LIBRARIES} SMI)