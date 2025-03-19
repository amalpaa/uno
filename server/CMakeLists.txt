cmake_minimum_required(VERSION 3.10)
project(Uno)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(uno 
                session.c
                main.c
                useful.c
                game.c
                antixss.c
                userinput.c)

target_link_libraries(uno websockets)
target_link_libraries(uno m)

configure_file(html_tags.txt ${CMAKE_BINARY_DIR} COPYONLY)