cmake_minimum_required (VERSION 2.6)

project(bridgexpc LANGUAGES C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

find_package(Libevent COMPONENTS libevent)

add_library(bridgexpc src/connection.c)
target_include_directories(bridgexpc PUBLIC include/)

if (LIBEVENT_FOUND)
    add_library(bridgexpc-libevent src/connection_libevent.c)
    target_include_directories(bridgexpc-libevent PUBLIC ${LIBEVENT_INCLUDE_DIRS})
    target_link_libraries(bridgexpc-libevent PUBLIC bridgexpc ${LIBEVENT_LIBRARIES})
endif()