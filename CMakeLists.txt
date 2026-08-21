cmake_minimum_required(VERSION 3.25)

project(CatalystEngine
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(CatalystEngine WIN32
    src/Main.cpp
    src/Renderer.cpp
    src/Renderer.h
)

target_compile_definitions(CatalystEngine
    PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

target_link_libraries(CatalystEngine
    PRIVATE
        d3d12.lib
        dxgi.lib
)

if (MSVC)
    target_compile_options(CatalystEngine PRIVATE
        /W4
        /permissive-
    )
endif()