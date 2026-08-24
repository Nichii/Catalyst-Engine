# CatalystEngine

A focused DirectX 12 renderer sample written in C++20. The project explores explicit GPU resource management and GPU-assisted visibility testing while keeping the frame flow small enough to inspect end-to-end.

## Implemented

- DirectX 12 device, command queue, swap chain, render-target, and fence setup
- HLSL vertex/pixel shader compilation at startup
- Instanced unit-cube rendering backed by a structured object buffer
- Compute-shader frustum culling for 1,000 object bounding spheres
- Explicit UAV, copy-source, and shader-readable resource transitions
- GPU visibility-mask consumption in the vertex shader
- CPU readback of the visibility mask for diagnostics
- Optional D3D12 debug layer in Debug builds
- CMake presets for MSVC

## Frame flow

1. Reset the command allocator and command list.
2. Transition the swap-chain image to RENDER_TARGET and clear it.
3. Dispatch Culling.hlsl over the object buffer.
4. Insert a UAV barrier and copy the visibility mask to a readback buffer.
5. Transition the visibility buffer to a shader-readable state.
6. Render instanced cubes; the vertex shader suppresses instances marked invisible.
7. Transition the swap-chain image to PRESENT, submit, present, and wait on the fence.
8. Map the completed readback buffer and count visible objects for diagnostics.

The culling pass uses a bounding-sphere test in clip space. Each compute thread processes one object and writes 1 or 0 to the visibility mask. Threads outside the object range return before accessing the structured buffer.

## Build

Requirements: Windows, a DirectX 12-capable GPU, Visual Studio with MSVC, and CMake 3.25+.

    cmake --preset x64-debug
    cmake --build out/build/x64-debug
    out\build\x64-debug\CatalystEngine.exe

For an optimized build, use cmake --preset x64-release followed by cmake --build out/build/x64-release.

## Scope and limitations

This is intentionally a renderer sample rather than a complete game engine. It performs GPU frustum visibility testing and uses the resulting mask to suppress culled instances, but still issues one instanced draw for the full object range. It does not yet implement GPU-generated indirect draw arguments, hierarchical-Z occlusion culling, bindless resources, materials, depth buffering, window resizing, or a production asset pipeline.

## Project layout

- src/main.cpp - Win32 entry point and message loop
- src/Renderer.h/.cpp - DirectX 12 setup, frame recording, resources, and passes
- src/Dx12Utils.h - shared HRESULT diagnostics
- shaders/Triangle.hlsl - instanced graphics shaders
- shaders/Culling.hlsl - frustum-culling compute shader
