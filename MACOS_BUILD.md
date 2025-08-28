# PixelShaper macOS Build Guide

This guide covers multiple ways to build PixelShaper for macOS.

## 🍎 Prerequisites

### For Local Building (Option 1)
- **macOS 10.15 or later**
- **Xcode Command Line Tools**: `xcode-select --install`
- **Homebrew**: Install from [https://brew.sh](https://brew.sh)

### For GitHub Actions (Option 2)
- GitHub repository with Actions enabled
- No local macOS machine required

### For Virtual Machine (Option 3)
- VMware or VirtualBox
- macOS installer (legal considerations apply)
- Sufficient system resources (8GB+ RAM recommended)

---

## 🚀 Build Methods

### Option 1: Local macOS Build

If you have access to a Mac, this is the fastest and most straightforward method:

1. **Install dependencies**:
   ```bash
   # Install Xcode Command Line Tools (if not already installed)
   xcode-select --install
   
   # Install Homebrew (if not already installed)
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   
   # Install required packages
   brew install cmake libpng
   ```

2. **Run the build script**:
   ```bash
   chmod +x build-macos.sh
   ./build-macos.sh
   ```

3. **Manual build** (alternative):
   ```bash
   mkdir build-macos && cd build-macos
   
   # Configure for universal binary (Intel + Apple Silicon)
   cmake .. \
       -DCMAKE_BUILD_TYPE=Release \
       -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
       -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
   
   # Build
   cmake --build . --config Release -j$(sysctl -n hw.ncpu)
   ```

### Option 2: GitHub Actions (Recommended)

Automated cloud building - no Mac required:

1. **Enable GitHub Actions** in your repository
2. **Push to main branch** or manually trigger the workflow
3. **Download artifacts** from the Actions tab

The workflow file is already created at: `.github/workflows/build-macos.yml`

**Benefits:**
- ✅ No macOS hardware required
- ✅ Automatic universal binary (Intel + Apple Silicon)
- ✅ Consistent build environment
- ✅ Easy to share builds

### Option 3: macOS Virtual Machine

For Windows/Linux users who want to build locally:

1. **Set up macOS VM**:
   - Use VMware Workstation/Fusion or VirtualBox
   - Install macOS (ensure you comply with Apple's license terms)
   - Enable hardware acceleration for better performance

2. **Follow Option 1 steps** inside the VM

---

## 📱 Output Files

After building, you'll get:

### Executable
- **Location**: `build-macos/build/PixerShaper`
- **Type**: Universal binary (runs on Intel and Apple Silicon Macs)
- **Usage**: `./PixerShaper`

### App Bundle
- **Location**: `build-macos/PixelShaper.app`
- **Type**: macOS application bundle
- **Usage**: Double-click to run, or drag to Applications folder

### Assets
- **Location**: `build-macos/build/assets/`
- **Contents**: All required PNG icons and example files

---

## 🔧 Technical Details

### Dependencies
- **OpenGL**: Native macOS OpenGL framework
- **Cocoa**: Native macOS UI framework
- **IOKit**: Hardware access framework
- **CoreVideo**: Video display framework
- **PNG**: Image format support
- **Native File Dialog Extended**: Cross-platform file dialogs

### Build Configuration
- **Target**: macOS 10.15+ (Catalina and later)
- **Architectures**: x86_64 (Intel) + arm64 (Apple Silicon)
- **Build Type**: Release (optimized)
- **C++ Standard**: C++20

### Universal Binary
The build creates a universal binary that runs natively on:
- Intel Macs (x86_64)
- Apple Silicon Macs (arm64/M1/M2/M3)

---

## 🐛 Troubleshooting

### Common Issues

**"xcode-select: error: tool 'xcodebuild' requires Xcode"**
```bash
xcode-select --install
```

**"brew: command not found"**
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

**CMake configuration fails**
```bash
# Update CMake
brew upgrade cmake

# Clean build directory
rm -rf build-macos && mkdir build-macos
```

**Library linking errors**
```bash
# Reinstall dependencies
brew reinstall libpng
```

### Performance Tips

- Use `-j$(sysctl -n hw.ncpu)` for parallel building
- Enable hardware acceleration in VM if using one
- Close unnecessary applications during build

---

## 📦 Distribution

### For App Store Distribution
Additional steps required:
1. Apple Developer account
2. Code signing
3. Notarization
4. App Store guidelines compliance

### For Direct Distribution
The created `.app` bundle can be:
- Zipped and shared directly
- Distributed via DMG installer
- Shared through GitHub releases

---

## 🔗 Next Steps

1. **Test the build** on different Mac models if possible
2. **Consider code signing** for better user experience
3. **Create DMG installer** for professional distribution
4. **Set up automated releases** via GitHub Actions

For questions or issues, refer to the main project documentation or open an issue on GitHub.
