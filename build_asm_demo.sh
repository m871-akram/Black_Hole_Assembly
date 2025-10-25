#!/bin/bash
# Build script for PhysicsASM_Demo

echo "╔════════════════════════════════════════════════════════════╗"
echo "║   Building Assembly-Optimized Physics Demo                ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Navigate to project directory
cd "$(dirname "$0")"

# Create build directory if it doesn't exist
mkdir -p cmake-build-debug
cd cmake-build-debug

# Configure CMake
echo "🔧 Configuring CMake..."
cmake .. || { echo "❌ CMake configuration failed"; exit 1; }

# Build the assembly demo
echo ""
echo "🔨 Building PhysicsASM_Demo..."
make PhysicsASM_Demo || { echo "❌ Build failed"; exit 1; }

echo ""
echo "✅ Build successful!"
echo ""
echo "To run the demo, execute:"
echo "  ./cmake-build-debug/PhysicsASM_Demo"
echo ""
echo "Or run it directly:"
read -p "Run PhysicsASM_Demo now? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║              Running PhysicsASM_Demo                       ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo ""
    ./PhysicsASM_Demo
fi

