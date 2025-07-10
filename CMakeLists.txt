cmake_minimum_required(VERSION 3.29)

# cmake options
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_CLANG_TIDY clang-tidy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_SKIP_RPATH ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions("-DLSFG_NO_DEBUG")
endif()

# subprojects
include(cmake/FetchDXVK.cmake)
include(cmake/FetchPeParse.cmake)

# Create vulkan interface library
add_library(vulkan INTERFACE)
target_link_libraries(vulkan INTERFACE -lvulkan)

add_subdirectory(lsfg-vk-gen)

# main project
project(lsfg-vk
    VERSION 0.0.1
    DESCRIPTION "lsfg-vk: LSFG on Linux through Vulkan"
    LANGUAGES CXX)

file(GLOB SOURCES
    "src/utils/*.cpp"
    "src/mini/*.cpp"
    "src/*.cpp"
)

add_library(lsfg-vk SHARED ${SOURCES})

target_include_directories(lsfg-vk
    PRIVATE include)
target_link_libraries(lsfg-vk
    PRIVATE lsfg-vk-gen vulkan)
target_compile_options(lsfg-vk PRIVATE
    -Weverything
    # disable compat c++ flags
    -Wno-pre-c++20-compat-pedantic
    -Wno-pre-c++17-compat
    -Wno-c++98-compat-pedantic
    -Wno-c++98-compat
    # disable other flags
    -Wno-missing-designated-field-initializers
    -Wno-shadow         # allow shadowing
    -Wno-switch-enum    # ignore missing cases
    -Wno-switch-default # ignore missing default
    -Wno-padded         # ignore automatic padding
    -Wno-exit-time-destructors # allow globals
    -Wno-global-constructors   # allow globals
    # required for vulkan
    -Wno-cast-function-type-strict
)

install(FILES "${CMAKE_BINARY_DIR}/liblsfg-vk.so"
    DESTINATION lib)
install(FILES "${CMAKE_SOURCE_DIR}/VkLayer_LS_frame_generation.json"
    DESTINATION share/vulkan/implicit_layer.d)
