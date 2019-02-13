cmake_minimum_required(VERSION 3.12)
project(sigmf_recoder)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH $CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/libsigmf)

add_subdirectory(libsigmf)

find_package(UHD   REQUIRED)
find_package(Boost REQUIRED system filesystem program_options)
#find_package(Sigmf REQUIRED)

add_executable(
        sigmf_recoder
        recorder.cpp
)

target_include_directories(
        sigmf_recoder
        PRIVATE
        ${UHD_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIR}
        ${SIGMF_INCLUDE_DIR}
)

target_link_libraries(
        sigmf_recoder
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