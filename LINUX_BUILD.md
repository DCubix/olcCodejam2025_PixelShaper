# PixelShaper Linux Build

This directory contains a Linux build of PixelShaper created using WSL (Windows Subsystem for Linux).

## Requirements

- WSL (Windows Subsystem for Linux) with Ubuntu
- X11 server for Windows (like VcXsrv, Xming, or Windows 11's WSLg)

## Running the Application

### Option 1: Using the convenience script
```bash
wsl bash ./run-linux.sh
```

### Option 2: Running directly
```bash
wsl bash -c "cd build-linux && ./build/PixerShaper"
```

### Option 3: From within WSL
```bash
cd /mnt/d/Projects/olcCodejam2025_shapes/build-linux
./build/PixerShaper
```

## Build Information

- **Platform**: Linux (x86_64)
- **Compiler**: GCC 11.4.0
- **Build Type**: Release
- **Dependencies**: 
  - OpenGL
  - X11
  - GTK3
  - PNG
  - Native File Dialog Extended
  - nlohmann/json

## Building from Source

To rebuild the Linux version:

1. Install dependencies:
```bash
sudo apt update
sudo apt install -y cmake g++ libgtk-3-dev libx11-dev libgl1-mesa-dev libglu1-mesa-dev libpng-dev
```

2. Configure and build:
```bash
mkdir -p build-linux
cd build-linux
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j4
```

## Files

- `build/PixerShaper` - Linux executable
- `build/assets/` - Required asset files
- All necessary libraries are dynamically linked

## Notes

- The executable is built as a dynamically linked ELF binary
- All required graphics and UI libraries are properly linked
- The application should work with any X11-compatible display server
- Assets are automatically copied to the build directory during compilation
