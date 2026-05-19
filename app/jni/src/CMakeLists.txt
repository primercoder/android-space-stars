cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(main SHARED
    main.cpp
)

target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../SDL/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../stars-on-space/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(main
    SDL2
    GLESv2
    EGL
    log
    android
)

# SDL2's CMake defines SDL2-static when built via add_subdirectory
