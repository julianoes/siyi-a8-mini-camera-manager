cmake_minimum_required(VERSION 3.10.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(camera-manager)

add_library(siyi
    siyi.cpp
    siyi_crc.cpp
)


add_executable(camera_manager
    camera_manager.cpp
)

find_package(MAVSDK REQUIRED)

target_link_libraries(camera_manager
    MAVSDK::mavsdk
    siyi
)

target_compile_options(camera_manager PRIVATE -Wall -Wextra)

add_executable(siyi_cli
    siyi_cli.cpp
)

target_link_libraries(siyi_cli
    siyi
)

target_compile_options(siyi_cli PRIVATE -Wall -Wextra)

include(CTest)

add_executable(siyi_test
    siyi_test.cpp
)

target_link_libraries(siyi_test
    siyi
)

target_compile_options(siyi_test PRIVATE -Wall -Wextra)

add_test(NAME siyi_test COMMAND siyi_test)

enable_testing()
