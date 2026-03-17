# LegacyMyLibs

Shared C++ utility libraries used across legacy portfolio projects. Originally written ~2008–2015, updated for VS2022 / x64 / Qt5 / C++17 / Boost 1.90 compatibility.

## Libraries

| Library | Description |
|---------|-------------|
| Effect | GLSL shader effect wrapper |
| fbo | OpenGL framebuffer object |
| GLBase | SDL + OpenGL window/context, textures, vertex sets |
| HPCLib | OpenCL wrappers for GPU compute |
| HPCNoise | Procedural noise via OpenCL |
| Math | Vectors, matrices, quaternions, transforms, noise math |
| Misc | Platform utils, atomics, hash, SIMD types, state machine |
| Remote | Remote debug interface |
| Render | OpenGL render context and pipeline |
| Scene | Scene graph (transform, group, drawable, camera) |
| Ugly | Qt utility widgets (Qt5) |
| Utils | Profiler and general utilities |
| Vice | Vice scene/animation format |
| hpalib | High-precision arithmetic library (C) |

## Usage

Used as a git submodule at `MyLibs/` within each project. Each project's `CMakeLists.txt` calls `add_subdirectory(MyLibs/<lib>)` for the libs it needs.

## Compatibility

- MSVC 2022 (v143), x64
- C++17
- Qt 5.15
- Boost 1.90
- OpenCL (CUDA 13.x)
