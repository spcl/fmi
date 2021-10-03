set (Boost_USE_STATIC_LIBS OFF)
find_package (Boost REQUIRED COMPONENTS unit_test_framework)
find_package(OpenMP REQUIRED)
include_directories (${Boost_INCLUDE_DIRS})


add_executable (Boost_Tests_run s3.cpp communicator.cpp)
target_link_libraries (Boost_Tests_run ${Boost_LIBRARIES} SMI OpenMP::OpenMP_CXX)