cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(rtsp-rebroadcast)

find_package(PkgConfig)
pkg_check_modules(GST REQUIRED
    gstreamer-1.0>=1.4
    gstreamer-rtsp-server-1.0>=1.4
)

add_executable(rtsp_rebroadcast rtsp_rebroadcast.cpp)

target_compile_options(rtsp_rebroadcast PRIVATE -Wall -Wextra)

target_include_directories(rtsp_rebroadcast SYSTEM PRIVATE
    ${GST_INCLUDE_DIRS}
)

target_link_libraries(rtsp_rebroadcast
    ${GST_LIBRARIES}
)

install(TARGETS rtsp_rebroadcast)
