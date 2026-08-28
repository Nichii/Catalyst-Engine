cmake_minimum_required(VERSION 3.25)

project(CatalystEngine
    VERSION 0.1.0
    LANGUAGES CXX
)

# Keep the C++ standard fixed across machines.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(CatalystEngine WIN32
    src/Main.cpp
    src/Application.h
    src/Application.cpp
    src/Core/Window.h
    src/Core/Window.cpp
    src/Core/Dx12Utils.h
    src/Renderer/Renderer.h
    src/Renderer/Renderer.cpp
    shaders/Triangle.hlsl
    shaders/Culling.hlsl
)

# Shaders are compiled by the app, so put the source files beside the executable.
add_custom_command(TARGET CatalystEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:CatalystEngine>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/Triangle.hlsl"
        "$<TARGET_FILE_DIR:CatalystEngine>/shaders/Triangle.hlsl"
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/Culling.hlsl"
        "$<TARGET_FILE_DIR:CatalystEngine>/shaders/Culling.hlsl"
)

target_compile_definitions(CatalystEngine
    PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
)

# These libraries come from the Windows SDK.
target_link_libraries(CatalystEngine
    PRIVATE
        d3d12.lib
        dxgi.lib
        d3dcompiler.lib
)

if (MSVC)
    target_compile_options(CatalystEngine PRIVATE
        /W4
        /permissive-
    )
endif()