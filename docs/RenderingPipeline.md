# Rendering pipeline

CatalystEngine records one direct command list per frame. The compute pass and graphics pass share the object data and visibility mask through a shader-visible descriptor heap.

```text
ObjectData SRV ──> Culling CS ──> Visibility UAV
					│
					├──> Visibility SRV ──> Vertex shader
					└──> Readback buffer ──> CPU diagnostics
```

## Resource states

The visibility buffer follows this sequence:

```text
GENERIC_READ -> UNORDERED_ACCESS -> COPY_SOURCE -> GENERIC_READ
```

The first transition allows the compute shader to write the UAV. A UAV barrier orders compute writes before the copy. The copy is completed before the buffer returns to `GENERIC_READ`, where the graphics vertex shader consumes it and the next frame begins.

The swap-chain image follows:

```text
PRESENT -> RENDER_TARGET -> PRESENT
```

## Review checklist

When capturing a demonstration, use a Debug build with the D3D12 debug layer enabled. The code currently renders a 32-column grid of 1,000 cubes. A useful screenshot or short capture should show the grid and include the visible-object count if a HUD or debugger is added.

For performance evidence, compare the GPU time of the culling-enabled path with a baseline that skips the visibility test. Record the GPU, driver, resolution, object count, and whether the debug layer was enabled. The current sample still submits a full instanced draw, so the culling pass primarily demonstrates GPU visibility data flow and rasterization suppression rather than complete draw-call compaction.
