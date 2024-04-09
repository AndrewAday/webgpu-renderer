# WebGPU-Renderer

## Building Natively

### Windows

```bash
# Debug
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Release
cmake -B build -DCMAKE_BUILD_TYPE=Release

# To build, use build/WebGPU-Renderer.sln
```

## Building for Web

Requires Chrome browser 113 or greater. Make sure hardware acceleration is enabled.

First [install emscripten](https://emscripten.org/docs/getting_started/downloads.html)

Then run:

```bash
emcmake cmake -B build-web
cmake --build build-web

# run local server to view the page
python -m http.server -d build-web
```