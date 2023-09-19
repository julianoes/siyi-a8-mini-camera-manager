cmake_minimum_required(VERSION 3.10.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(camera-manager)

add_executable(camera_manager
    camera_manager.cpp
)

find_package(MAVSDK REQUIRED)

target_link_libraries(camera_manager
    MAVSDK::mavsdk
)

add_compile_options(camera_server PRIVATE -Wall -Wextra)
