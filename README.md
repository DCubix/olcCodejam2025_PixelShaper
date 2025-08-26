# olcPixelGameEngine Cross-Platform CMake Project

This project demonstrates how to set up a cross-platform CMake project using the olcPixelGameEngine by OneLoneCoder (javidx9).

## Features

- Cross-platform support (Windows, Linux, macOS)
- CMake build system
- Automatic platform detection and library linking
- Basic example application with graphics primitives

## Prerequisites

### All Platforms
- CMake 3.16 or higher
- C++14 compatible compiler

### Windows
- Visual Studio 2017 or newer (MSVC)
- OR MinGW-w64 with GCC

### Linux
- GCC or Clang
- Development packages:
  ```bash
  # Ubuntu/Debian
  sudo apt-get install build-essential cmake libx11-dev libpng-dev

  # Fedora/RHEL/CentOS
  sudo dnf install gcc-c++ cmake libX11-devel libpng-devel

  # Arch Linux
  sudo pacman -S gcc cmake libx11 libpng
  ```

### macOS
- Xcode Command Line Tools
- CMake (via Homebrew recommended):
  ```bash
  brew install cmake
  ```

## Building

### Quick Build (All Platforms)

1. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```

2. Configure with CMake:
   ```bash
   cmake ..
   ```

3. Build the project:
   ```bash
   cmake --build .
   ```

4. Run the executable:
   ```bash
   # Windows
   .\bin\Debug\olcPixelGameEngine_Project.exe
   
   # Linux/macOS
   ./bin/olcPixelGameEngine_Project
   ```

### Platform-Specific Build Instructions

#### Windows (Visual Studio)
```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

#### Windows (MinGW)
```cmd
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

#### Linux
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

#### macOS
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```
