# CircleP
Another parallel algorithm to generate circle 

C++ OpenGL starter implementation

This version computes only one octant on CPU, then uses an OpenGL vertex shader to generate the 8 symmetric positions.
maingl.ccp change it to main.cpp then run it
as OpenGl project it uses OpenGL for parallesim.

# GPU Eight-Octant Symmetry Circle

This program:

1. Generates only the 0°–45° base octant with the midpoint circle algorithm.
2. Uploads that octant to one OpenGL vertex buffer.
3. Uses eight homogeneous symmetry matrices `R[0..7]` in the vertex shader.
4. Builds translation matrix `T` from the circle center.
5. Computes `TR = T * R[gl_InstanceID]`.
6. Dispatches all eight octants together with one call:

```cpp
glDrawArraysInstanced(GL_POINTS, 0, baseOctantPointCount, 8);
```

## Dependencies

- C++17 compiler
- CMake 3.16+
- OpenGL
- GLFW 3
- GLEW

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libglew-dev
```

### macOS with Homebrew

```bash
brew install cmake glfw glew
```

### Windows with vcpkg

```powershell
vcpkg install glfw3 glew
```

Configure CMake with your vcpkg toolchain file when necessary.

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Run

Linux/macOS:

```bash
./build/gpu_symmetry_circle
```

Specify another radius:

```bash
./build/gpu_symmetry_circle 350
```

On a multi-configuration Windows generator:

```powershell
.\build\Release\gpu_symmetry_circle.exe 350
```

Press Escape to exit.

## Important interpretation

"At once" means one instanced OpenGL draw submission containing eight instances.
The GPU scheduler distributes the resulting vertex-shader invocations across
available execution units. It does not guarantee that every octant instruction
begins on the exact same clock cycle.
