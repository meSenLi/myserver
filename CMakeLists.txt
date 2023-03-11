cmake_minimum_required(VERSION 3.22)
project(myserver)

set(CXX_STANDARD_REQUIRED ON)
set(CXX_STANDARD 20)


set(CMAKE_BUILD_TYPE debug)
set(EXECUTABLE_OUTPUT_PATH bin)

add_subdirectory(src)

set(SOURCES "main.cpp")


add_executable(run ${SOURCES})
target_link_libraries(run webserver)
