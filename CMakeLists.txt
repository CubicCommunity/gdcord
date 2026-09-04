cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(gdcord VERSION 1.0.0)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_library(${PROJECT_NAME} STATIC ${SOURCES})

if(NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE GEODE_MOD_ID="_gdc")
target_compile_definitions(${PROJECT_NAME} PRIVATE GDC_VERSION=${PROJECT_VERSION})

target_link_libraries(${PROJECT_NAME} geode-sdk)

CPMAddPackage("gh:GlobedGD/argon@1.5.1")
target_link_libraries(${PROJECT_NAME} argon)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
