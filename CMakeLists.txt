cmake_minimum_required(VERSION 3.25)
project(3d LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O3")

option(VALIDATION "enable validation layer" OFF)

if(VALIDATION)
    add_compile_definitions(VALIDATION)
endif()

add_compile_definitions(VK_NO_PROTOTYPES)

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_DEPRECATE)
endif()

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/main.c
)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)

add_subdirectory(external/JoltC)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME} joltc SDL3::SDL3 Vulkan::Headers)
if(UNIX)
    target_link_libraries(${CMAKE_PROJECT_NAME} m)
endif()
if(NOT VK_NO_PROTOTYPES)
    target_link_libraries(${CMAKE_PROJECT_NAME} Vulkan::Vulkan)
endif()
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include ${CMAKE_CURRENT_LIST_DIR}/external/cglm/include)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE CMAKE_EXPORT_COMPILE_COMMANDS=ON)

if(WIN32)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${CMAKE_PROJECT_NAME}> COMMAND_EXPAND_LISTS $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>)
endif()