#!/bin/bash
# macOS Build Script for PixelShaper

echo "🍎 Building PixelShaper for macOS..."

# Check if we're on macOS
if [[ "$(uname)" != "Darwin" ]]; then
    echo "❌ This script must be run on macOS"
    exit 1
fi

# Check if Xcode Command Line Tools are installed
if ! command -v clang &> /dev/null; then
    echo "❌ Xcode Command Line Tools not found. Please install with:"
    echo "   xcode-select --install"
    exit 1
fi

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "❌ Homebrew not found. Please install from https://brew.sh"
    exit 1
fi

# Install dependencies
echo "📦 Installing dependencies..."
brew install cmake libpng

# Create build directory
echo "🔨 Configuring build..."
mkdir -p build-macos
cd build-macos

# Configure CMake with universal binary support
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15

if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed"
    exit 1
fi

# Build the project
echo "🔨 Building..."
cpu_count=$(sysctl -n hw.ncpu)
cmake --build . --config Release -j$cpu_count

if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi

# Create macOS app bundle
echo "📱 Creating app bundle..."
mkdir -p PixelShaper.app/Contents/MacOS
mkdir -p PixelShaper.app/Contents/Resources

# Copy executable
cp build/PixerShaper PixelShaper.app/Contents/MacOS/

# Copy assets
cp -r build/assets PixelShaper.app/Contents/Resources/

# Create Info.plist
cat > PixelShaper.app/Contents/Info.plist << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>PixerShaper</string>
    <key>CFBundleIdentifier</key>
    <string>com.dcubix.pixelshaper</string>
    <key>CFBundleName</key>
    <string>PixelShaper</string>
    <key>CFBundleDisplayName</key>
    <string>PixelShaper</string>
    <key>CFBundleVersion</key>
    <string>1.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0.0</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleSignature</key>
    <string>PSHR</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
</dict>
</plist>
EOF

echo "✅ Build completed successfully!"
echo ""
echo "📁 Output files:"
echo "   • Executable: build-macos/build/PixerShaper"
echo "   • App Bundle: build-macos/PixelShaper.app"
echo ""
echo "🚀 To run:"
echo "   • Double-click PixelShaper.app"
echo "   • Or run: ./build/PixerShaper"
