cmake_minimum_required(VERSION 3.12)
project(sigmf_recorder)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH $CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/libsigmf)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++ -fno-stack-protector")

add_subdirectory(libsigmf)

find_package(UHD   REQUIRED)
find_package(Boost REQUIRED system filesystem program_options date_time thread)

add_executable(
        sigmf_recorder
        recorder.cpp
)

target_include_directories(
        sigmf_recorder
        PRIVATE
        ${UHD_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR}
        ${SIGMF_INCLUDE_DIR}
)

target_link_libraries(
        sigmf_recorder
        libsigmf
        ${UHD_LIBRARIES}
        ${Boost_SYSTEM_LIBRARY_RELEASE}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY}
)

## benchmark test
add_executable(
        benchmark
        benchmark_rate.cpp
)

target_include_directories(
        benchmark
        PRIVATE
        ${UHD_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR}
        ${SIGMF_INCLUDE_DIR}
)

target_link_libraries(
        benchmark
        ${UHD_LIBRARIES}
        ${Boost_SYSTEM_LIBRARY_RELEASE}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_ALGORITHM_LIBRARY}
        ${Boost_THREAD_LIBRARY}
        ${Boost_DATE_TIME_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY}
)

## benchmark test
add_executable(
        rx_file
        rx_samples_to_file.cpp
)

target_include_directories(
        rx_file
        PRIVATE
        ${UHD_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR}
        ${SIGMF_INCLUDE_DIR}
)

target_link_libraries(
        rx_file
        ${UHD_LIBRARIES}
        ${Boost_SYSTEM_LIBRARY_RELEASE}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_ALGORITHM_LIBRARY}
        ${Boost_THREAD_LIBRARY}
        ${Boost_DATE_TIME_LIBRARY}
)

## manifest generator
add_executable(
        manifest
        manifest.cpp
)

target_include_directories(
        manifest
        PRIVATE
        ${UHD_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR}
        ${SIGMF_INCLUDE_DIR}
)

target_link_libraries(
        manifest
        libsigmf
        ${UHD_LIBRARIES}
        ${Boost_SYSTEM_LIBRARY_RELEASE}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
)

## manifest generator
add_executable(
        json_concat
        json_concat.cpp
)

target_include_directories(
        json_concat
        PRIVATE
        ${UHD_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR}
        ${SIGMF_INCLUDE_DIR}
)

target_link_libraries(
        json_concat
        libsigmf
        ${UHD_LIBRARIES}
        ${Boost_SYSTEM_LIBRARY_RELEASE}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
)



#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()