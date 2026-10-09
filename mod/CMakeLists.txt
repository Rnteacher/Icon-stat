cmake_minimum_required(VERSION 3.29)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(GEODE_BUNDLE_PDB ON)

project(IconStat VERSION 1.0.0)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if(NOT DEFINED ENV{GEODE_SDK})
  set(ENV{GEODE_SDK} "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/geode")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../vendor/argon ${CMAKE_CURRENT_BINARY_DIR}/argon)
if(MSVC)
  # MSVC 19.44 ICEs while optimizing Arc's Pollable template in Argon 1.5.1.
  # Keep only this small dependency at /Od; Icon Stat itself remains optimized.
  target_compile_options(argon PRIVATE /Od)
endif()
target_include_directories(${PROJECT_NAME} PRIVATE src)
setup_geode_mod(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} argon)
