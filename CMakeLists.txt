cmake_minimum_required(VERSION 3.12)
project(sigmf_recoder)

set(CMAKE_CXX_STANDARD 14)

find_package(UHD   REQUIRED)
find_package(Boost REQUIRED system filesystem program_options)
find_package(Sigmf REQUIRED)

add_executable(
        sigmf_recoder
        recorder.cpp
)

target_include_directories(
        sigmf_recoder
        PRIVATE
        ${UHD_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR}
        ${SIGMF_INCLUDE_DIRS}
)

target_link_libraries(
        sigmf_recoder
        ${UHD_LIBRARIES}
        ${Boost_SYSTEM_LIBRARY_RELEASE}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
)


