cmake_minimum_required(VERSION 3.10) 

project(server C)

set(CMAKE_C_STANDARD 11) 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wextra")

find_package(PkgConfig) 

pkg_check_modules(LIBURING liburing)

include_directories(${LIBURING_INCLUDE_DIRS}) 

link_directories(${LIBURING_LIBRARY_DIRS})

add_executable(server src/server.c)

target_link_libraries(server ${LIBURING_LIBRARIES})
