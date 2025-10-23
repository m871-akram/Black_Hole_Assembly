# Project Architecture

This document explains how the project orchestrates CPU and GPU resources, how data flows through the system, and how we structure concurrency, memory, and performance. It is intended for developers extending the pipeline, debugging performance, or porting to new GPU backends.

- Target platform: macOS
- Language: C\+\+
- Supported GPU backends: CUDA \(Linux\/Windows\), OpenCL \(legacy\), Metal \(macOS, recommended\), Vulkan\-compute \(optional\)

## 1. High\-Level Overview

The system is a staged pipeline:

1. Ingestion: load or generate input data on CPU.
2. Preprocess: CPU transforms, tiling, normalization, and upload packing.
3. Transfer: host→device \(H2D\) copy \(async\).
4. Compute: GPU kernels \(batched\).
5. Transfer: device→host \(D2H\) copy \(async\).
6. Postprocess: CPU reduction, validation, serialization.

Key goals:
- Keep GPU saturated by overlapping transfer and compute.
- Keep CPU cores busy with preprocessing and postprocessing.
- Minimize copies via pinned\/page\-locked buffers and reuse of device allocations.
- Batch small jobs to amortize kernel launch overhead.

## 2. Runtime Data Flow \(ASCII Scheme\)

+--------------------+     +------------------+     +----------------------+     +-----------------+     +-------------------+
|  Input Producers   | --> |  CPU Preprocess  | --> |  H2D Transfer Queue  | --> |  GPU Compute     | --> |  D2H Transfer      |
|  \(I\/O, gen\)     |     |  \(tiling, pack\)|     |  \(pinned buffers\)  |     |  \(kernels, q\)  |     |  Queue             |
+--------------------+     +------------------+     +----------------------+     +-----------------+     +-------------------+
                                                                                                                |
                                                                                                                v
                                                                                                       +------------------+
                                                                                                       | CPU Postprocess  |
                                                                                                       | \(reduce, write\)|
                                                                                                       +------------------+

Concurrency:
- CPU thread pool feeds preprocess and postprocess.
- GPU command queues\/streams overlap H2D, kernels, D2H.
- Double\/triple buffering between stages.

## 3. CPU Architecture

- Threading: std::thread\/std::async or a pool \(e.g., TBB\/std::jthread\) for:
  - I\/O and decode
  - Preprocess \(vectorized where possible: SSE\/AVX\/NEON\)
  - Postprocess \(reductions, formatting\)
- Job Graph:
  - Each input becomes a `Job` with states: `Ready` → `Uploading` → `Computing` → `Downloading` → `Completed`.
  - A scheduler batches compatible jobs into `Batch` objects.
- Queues:
  - `cpu_preprocess_q` → `h2d_q` → `gpu_q` → `d2h_q` → `cpu_postprocess_q`
  - Lock\-free MPSC structures where possible.

## 4. GPU Architecture

Abstraction:
- `IGpuContext`: device, queues\/streams, allocators.
- `IGpuBuffer`: device memory handle.
- `IKernel`: compiled compute kernel with launch parameters.
- `ICommandList`: records transfers and dispatches for submission.

Backends:
- Metal \(macOS\):
  - 1 `MTLDevice`, N `MTLCommandQueue` \(typically 2–3\).
  - Use `MTLHeap` for suballocation; `MTLBuffer` for device storage.
  - Synchronization via command buffer completion handlers.
- CUDA \(non\-macOS\):
  - Multiple streams per device to overlap memcpy and kernel \(e.g., stream 0 for H2D, 1..k for compute, last for D2H\).
  - Events for dependencies; use pinned host memory `cudaHostAlloc`.
- OpenCL \(legacy\):
  - One context, multiple command queues; out\-of\-order execution if supported.

Kernel dispatch policy:
- Fixed workgroup size tuned per kernel.
- Batch N items to reach target occupancy.
- Auto\-tuner records timing per batch size and adapts.

## 5. Memory Model

Host memory:
- Pinned \(page\-locked\) pools for H2D\/D2H to enable async DMA.
- Staging buffers sized for 2–3 frames\/batches to allow overlap.

Device memory:
- Long\-lived pools for reusable tensors\/tiles \(avoid frequent alloc\/free\).
- Transient scratch for kernels via suballocator \(ring or buddy\).

Zero\/Unified memory:
- Prefer explicit copies for predictability.
- On Apple Silicon: use `storageModeShared` for small control buffers; `storageModePrivate` for bulk compute.

Copy strategies:
- Double buffering per stage:
  - While GPU computes on `dev_buf[i]`, CPU uploads into `dev_buf[i^1]`.

## 6. Scheduling, Batching, and Overlap \(ASCII Timeline\)

Time ───▶

CPU Pre  | [####]      [####]      [####]
H2D      |    [==]        [==]        [==]
GPU Comp |       [██████]    [██████]    [██████]
D2H      |             [==]        [==]        [==]
CPU Post |                 [###]       [###]       [###]

Goal: no idle gaps; queues should stay non\-empty.

Batching rules:
- Group by kernel, shape, and data layout.
- Cap by device memory watermark and max workgroups.
- Split oversized inputs into tiles with halo padding.

## 7. Error Handling and Telemetry

- Each command submission tagged with `JobId` and `BatchId`.
- Timing:
  - CPU: `std::chrono` spans per stage.
  - GPU: backend timestamp queries \(Metal counters, CUDA events\).
- Health:
  - Queue depths exported as gauges.
  - Backpressure: if `gpu_q` exceeds high watermark, slow ingestion.

## 8. Build and Runtime Configuration

- CMake options:
  - `-DGPU_BACKEND=Metal|CUDA|OpenCL|None`
  - `-DENABLE_PROFILING=ON`
  - `-DENABLE_ASSERTS=ON`
- macOS \(recommended\):
  - Enable Metal: link `Metal`, `MetalKit`, `MetalPerformanceShaders` if used.
- Environment:
  - `GPU_QUEUE_COUNT` \(default 3\)
  - `BATCH_TARGET_MS` \(auto\-tuner horizon\)
  - `H2D_D2H_BUFFER_COUNT` \(2 or 3\)

## 9. Typical Control Flow \(Pseudocode\)

CPU:
- Pop N items from `cpu_preprocess_q`, preprocess in parallel.
- Acquire pinned host buffers, pack batch payload.
- Submit H2D copies to transfer queue \(async\).
- Enqueue kernel dispatch with dependencies on H2D completion.
- Enqueue D2H copies dependent on compute completion.
- Postprocess upon D2H completion callback.

GPU:
- Command queue executes: H2D → Kernels → D2H per batch.
- Events\/fences ensure ordering between streams\/queues.

## 10. Extensibility

To add a new GPU kernel:
1. Implement `IKernel` for the backend \(Metal shader or CUDA kernel\).
2. Register kernel metadata \(preferred block size, shared memory needs\).
3. Provide CPU fallback for `GPU_BACKEND=None`.
4. Update batcher rules if input shape changes.

To add a new stage:
1. Define a `Stage` node with input\/output contracts.
2. Allocate buffers via the device allocator.
3. Wire dependencies via events and queue submission.

## 11. ASCII Component Diagram

+-------------------+            +--------------------+            +--------------------+
|    Scheduler      |  submits   |   GPU Abstraction  |  manages   |  Backend Driver    |
|  \(jobs, batches\)|----------->| \(ctx, queues, mem\)|----------->| \(Metal\/CUDA\/CL\) |
+-------------------+            +--------------------+            +--------------------+
         |                                 |                                 |
         v                                 v                                 v
+-------------------+            +--------------------+            +--------------------+
| CPU Pre\/Post     |            |  Buffer Allocator  |            |   Kernel Library   |
| \(thread pool\)   |            | \(host\/device\)    |            | \(shaders\/ptx\/il\)|
+-------------------+            +--------------------+            +--------------------+

## 12. Performance Checklist

- Keep at least 2 in\-flight batches.
- Use pinned host memory for all transfers.
- Avoid per\-item kernel launches; prefer batched dispatch.
- Reuse device buffers; avoid allocator churn.
- Profile with GPU counters; verify overlap in timelines.
- Validate that CPU preprocess does not starve GPU.

## 13. File and Module Map \(example\)

- `src\/runtime\/scheduler\.hpp,\.cpp`: job graph, batching, backpressure
- `src\/runtime\/thread_pool\.hpp,\.cpp`: CPU workers
- `src\/gpu\/gpu_context\.*`: device creation, queues
- `src\/gpu\/buffers\.*`: host\/device allocators
- `src\/gpu\/kernels\/\*`: backend kernels
- `src\/pipeline\/stages\.*`: preprocess\/postprocess
- `include\/config\.hpp`: compile\-time flags

## 14. Notes for macOS \(Metal\)

- Prefer `storageModePrivate` for device buffers, `blitCommandEncoder` for copies.
- Use separate command buffers per batch; chain with `addCompletedHandler`.
- For Apple Silicon, unified memory can help for small control data, but large data should be `Private` with explicit blits for bandwidth.

