cmake_minimum_required(VERSION 3.10.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(camera-manager)

add_library(siyi
    siyi_protocol.cpp
    siyi_crc.cpp
)

install(TARGETS siyi)

add_executable(camera_manager
    camera_manager.cpp
)

install(TARGETS camera_manager)
install(FILES mavlink_ftp_root/siyi_a8_mini.xml DESTINATION share/mavlink_ftp_root)

find_package(MAVSDK REQUIRED)

target_link_libraries(camera_manager
    MAVSDK::mavsdk
    siyi
    stdc++fs
)

target_compile_options(camera_manager PRIVATE -Wall -Wextra)

add_executable(siyi_cli
    siyi_cli.cpp
)

target_link_libraries(siyi_cli
    siyi
)

target_compile_options(siyi_cli PRIVATE -Wall -Wextra)

install(TARGETS siyi_cli)

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
