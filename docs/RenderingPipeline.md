# Rendering pipeline

CatalystEngine records one direct command list per frame. The compute pass and graphics pass share object data through a shader-visible descriptor heap, while the compute pass writes an indirect command stream consumed by `ExecuteIndirect`.

```text
ObjectData SRV ──> Culling CS ──> Indirect command UAV ──> ExecuteIndirect
					│                          │
					└──> Command counter UAV ──┘

ObjectData SRV ────────────────────────────────────────> Vertex shader
```

## Resource states

The generated command and count buffers follow this sequence:

```text
COMMON/INDIRECT_ARGUMENT -> UNORDERED_ACCESS -> INDIRECT_ARGUMENT
```

The first transition allows the compute shader to write the UAV. A UAV barrier orders compute writes before the buffers return to `INDIRECT_ARGUMENT` for `ExecuteIndirect`. The count buffer is cleared with a UAV clear each frame; no visibility readback is performed.

The swap-chain image follows:

```text
PRESENT -> RENDER_TARGET -> PRESENT
```

## Review checklist

When capturing a demonstration, use a Debug build with the D3D12 debug layer enabled. The code currently renders 10,000 cubes in a 20 x 20 x 25 grid. A useful capture should show the grid, camera controls, and the Output-window mode diagnostics.

For performance evidence, compare the GPU indirect path with the CPU baseline while recording the GPU, driver, resolution, object count, and debug-layer state. The indirect path submits one `ExecuteIndirect` call; the baseline records one indexed draw per object.
