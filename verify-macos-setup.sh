#!/bin/bash
# Verify macOS build configuration

echo "🔍 Verifying macOS build setup..."

# Check if GitHub Actions workflow exists
if [ -f ".github/workflows/build-macos.yml" ]; then
    echo "✅ GitHub Actions workflow found"
else
    echo "❌ GitHub Actions workflow missing"
    exit 1
fi

# Check if build script exists
if [ -f "build-macos.sh" ]; then
    echo "✅ macOS build script found"
else
    echo "❌ macOS build script missing"
    exit 1
fi

# Check CMakeLists.txt for macOS support
if grep -q "APPLE" CMakeLists.txt && grep -q "COCOA_FRAMEWORK" CMakeLists.txt; then
    echo "✅ CMakeLists.txt has macOS support"
else
    echo "❌ CMakeLists.txt missing macOS configuration"
    exit 1
fi

# Check if documentation exists
if [ -f "MACOS_BUILD.md" ]; then
    echo "✅ macOS documentation found"
else
    echo "❌ macOS documentation missing"
    exit 1
fi

echo ""
echo "🎉 macOS build setup verification complete!"
echo ""
echo "📋 Next steps:"
echo "   1. Push to GitHub to trigger the macOS build action"
echo "   2. Check the Actions tab for build results"
echo "   3. Download artifacts when build completes"
echo ""
echo "🍎 Or build locally on a Mac with:"
echo "   chmod +x build-macos.sh && ./build-macos.sh"
