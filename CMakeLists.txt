cmake_minimum_required(VERSION 3.15)
project(hooks C)

set(CMAKE_C_STANDARD 11)

include_directories($ENV{CEDEV}/include)

add_custom_target(Make ALL $(MAKE) WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(hooks
        src/hookman.h
        src/hookman.c src/test.c)
