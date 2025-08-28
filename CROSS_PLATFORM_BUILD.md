# PixelShaper - Cross-Platform Build Guide

PixelShaper is a cross-platform application that can be built for Windows, Linux, and macOS.

## 🎯 Quick Start

| Platform | Status | Method | Documentation |
|----------|--------|--------|---------------|
| **Windows** | ✅ Ready | VS Code + CMake | Built-in tasks |
| **Linux** | ✅ Ready | WSL + CMake | [LINUX_BUILD.md](LINUX_BUILD.md) |
| **macOS** | 🔄 Setup | GitHub Actions / Local Mac | [MACOS_BUILD.md](MACOS_BUILD.md) |

## 🏗️ Build Status

### Windows Build
- **Status**: ✅ Complete and tested
- **Location**: `build/build/Debug/PixerShaper.exe` or `build/build/Release/PixerShaper.exe`
- **Run**: Use VS Code "Run Application" task or execute directly

### Linux Build  
- **Status**: ✅ Complete and tested
- **Location**: `build-linux/build/PixerShaper`
- **Run**: `wsl bash ./run-linux.sh`
- **Dependencies**: All linked (OpenGL, X11, GTK3, PNG)

### macOS Build
- **Status**: 🔄 Configured but needs macOS to build
- **Options**: 
  - **GitHub Actions** (recommended): Automatic cloud build
  - **Local Mac**: Use `./build-macos.sh` script
  - **macOS VM**: Run script in virtual machine

## 🚀 Running the Application

### Windows
```bash
# Option 1: VS Code Task
Ctrl+Shift+P → "Tasks: Run Task" → "Run Application"

# Option 2: Direct execution
./build/build/Release/PixerShaper.exe
```

### Linux (via WSL)
```bash
# Option 1: Convenience script
wsl bash ./run-linux.sh

# Option 2: Direct execution
wsl bash -c "cd build-linux && ./build/PixerShaper"
```

### macOS
```bash
# After building locally
./build-macos/PixelShaper.app

# Or run executable directly
./build-macos/build/PixerShaper
```

## 📁 Project Structure

```
olcCodejam2025_shapes/
├── src/                     # Source code
├── assets/                  # Icons and resources
├── build/                   # Windows build output
├── build-linux/            # Linux build output
├── .github/workflows/       # GitHub Actions for macOS
├── CMakeLists.txt          # Cross-platform build config
├── run-linux.sh           # Linux run script
├── build-macos.sh          # macOS build script
├── LINUX_BUILD.md          # Linux build documentation
├── MACOS_BUILD.md          # macOS build documentation
└── README.md               # This file
```

## 🔧 Development

### Prerequisites
- **CMake 3.16+**
- **C++20 compatible compiler**
- **Platform-specific dependencies** (see individual build guides)

### Adding New Features
1. Modify source code in `src/`
2. Update assets in `assets/` if needed
3. Test on available platforms
4. Update documentation

### Cross-Platform Notes
- **File paths**: Use `std::filesystem` for cross-platform compatibility
- **UI libraries**: Native File Dialog Extended handles platform differences
- **Graphics**: olcPixelGameEngine abstracts platform-specific graphics code
- **Building**: CMake handles platform detection and library linking

## 📦 Distribution

### For End Users
- **Windows**: Distribute `PixerShaper.exe` with `assets/` folder
- **Linux**: Distribute `PixerShaper` binary with `assets/` folder  
- **macOS**: Distribute `.app` bundle or `.dmg` installer

### For Developers
- Clone repository
- Follow platform-specific build instructions
- Use provided scripts for convenience

## 🤝 Contributing

1. **Fork** the repository
2. **Create** a feature branch
3. **Test** on available platforms
4. **Submit** a pull request

For macOS testing, use GitHub Actions to verify builds work correctly.

## 📄 License

See the project license file for details.

---

**Need help?** Check the platform-specific documentation:
- [Linux Build Guide](LINUX_BUILD.md)
- [macOS Build Guide](MACOS_BUILD.md)
