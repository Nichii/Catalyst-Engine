# CatalystEngine

CatalystEngine is a small DirectX 12 rendering project I am using to learn more about GPU-driven rendering. The current focus is frustum culling and generating indirect draw commands on the GPU instead of having the CPU submit one draw for every object.

It is intentionally still a renderer demo, not a full game engine. Keeping the scope small makes it easier to follow the data flow from the compute shader to the final draw.

## What is working

- DirectX 12 device, command queue, swap chain, render-target, and fence setup
- HLSL vertex/pixel shader compilation at startup
- Instanced unit-cube rendering backed by a structured object buffer
- Compute-shader frustum culling for 10,000 object bounding spheres
- Explicit UAV and indirect-argument resource transitions
- GPU-compacted indirect command generation and `ExecuteIndirect` submission
- Resize-aware depth buffering and camera orbit/zoom controls
- Pressing F1 button to change between GPU indirect VS CPU baseline comparison mode (use VS's output window to verify mode and performance)
- Optional D3D12 debug layer in Debug builds
- CMake presets for MSVC

## How a frame works

1. Reset the command allocator and command list.
2. Transition the indirect command and count buffers to UAV state.
3. Clear the GPU command counter.
4. Run one culling thread for each object.
5. Each visible object reserves a slot and writes one indirect command.
6. Insert a UAV barrier, then transition the generated buffers to `INDIRECT_ARGUMENT`.
7. Bind the camera, object buffer, geometry, and graphics pipeline.
8. Call `ExecuteIndirect`. The GPU-generated count determines how many commands run.
9. Transition the back buffer to `PRESENT`, submit the work, and present it.

The culling pass uses a bounding-sphere test in clip space. Each compute thread handles one object, and visible objects append commands using an atomic counter. The vertex shader uses the object index from each command to load the matching transform. There is no visibility readback in the normal frame path.

## Controls and comparison

- Left/right arrow: orbit the camera
- Up/down arrow: zoom in and out
- W/S: move forward/backward relative to the camera
- A/D: move left/right relative to the camera
- F1: switch between GPU indirect mode and the CPU baseline

The program writes a short diagnostic message to the Visual Studio Output window once per second. It includes the current mode, object count, CPU draw count, and FPS. For actual CPU/GPU timing, capture both modes in PIX or Visual Studio Graphics Diagnostics.

The CPU baseline submits one draw per object. The GPU path submits one `ExecuteIndirect` call while the compute shader builds the command list and count. At 10,000 simple cubes, the indirect path is not guaranteed to be faster in every measurement, but the most interesting part is how the CPU submission work scales the more objects there are.

## Build

Requirements: Windows, a DirectX 12-capable GPU, Visual Studio with MSVC, and CMake 3.25+.

    cmake --preset x64-debug
    cmake --build out/build/x64-debug
    out\build\x64-debug\CatalystEngine.exe

For an optimized build:

```powershell
cmake --preset x64-release
cmake --build out/build/x64-release
```

Shaders are compiled when the application starts and copied beside the executable by the post-build step.

## Controls and comparison

- Left/right arrow: orbit the camera
- Up/down arrow: zoom the camera
- F1: toggle GPU indirect mode and the CPU baseline

Diagnostics report mode, object count, CPU draw submissions, and FPS once per second through `OutputDebugStringA`. Capture CPU and GPU timings with PIX or Visual Studio Graphics Diagnostics.

## Scope and limitations

This project is deliberately focused on one rendering technique. It does not have hierarchical-Z occlusion culling, GPU LOD selection, bindless resources, materials, multi-frame buffering, or a production asset pipeline yet. The renderer also waits for the previous frame, which keeps synchronization easy to inspect but is not how a finished engine would normally be structured.

## Project layout

- src/main.cpp - Win32 entry point and message loop
- src/Renderer.h/.cpp - DirectX 12 setup, frame recording, resources, and passes
- src/Dx12Utils.h - shared HRESULT diagnostics
- shaders/Triangle.hlsl - instanced graphics shaders
- shaders/Culling.hlsl - frustum-culling compute shader
