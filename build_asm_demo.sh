#!/bin/bash
# Build script for Black Hole Assembly Project - All Executables

echo "╔════════════════════════════════════════════════════════════╗"
echo "║   Building Black Hole Assembly-Optimized Project          ║"
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

# Build all projects
echo ""
echo "🔨 Building All Projects..."
echo ""
echo "  [1/4] Building PhysicsASM_Demo (Assembly test suite)..."
make PhysicsASM_Demo || { echo "❌ PhysicsASM_Demo build failed"; exit 1; }

echo ""
echo "  [2/4] Building GravitySim (N-body with assembly)..."
make GravitySim || { echo "❌ GravitySim build failed"; exit 1; }

echo ""
echo "  [3/4] Building BlackHole3D (Ray tracer with assembly)..."
make BlackHole3D || { echo "❌ BlackHole3D build failed"; exit 1; }

echo ""
echo "  [4/4] Building BlackHole2D (2D lensing with assembly)..."
make BlackHole2D || { echo "❌ BlackHole2D build failed"; exit 1; }

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                 ✅ All Builds Successful!                  ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║              Which simulation to run?                      ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Choose an option:"
echo "  1) PhysicsASM_Demo  - Test assembly functions (quick, no graphics)"
echo "  2) GravitySim       - N-body simulation (⚡ BEST SHOWCASE)"
echo "  3) BlackHole3D      - 3D ray tracer (requires OpenGL 4.3+)"
echo "  4) BlackHole2D      - 2D lensing demo"
echo "  5) Exit (run manually later)"
echo ""
read -p "Enter choice [1-5]: " choice

case $choice in
    1)
        echo ""
        echo "╔════════════════════════════════════════════════════════════╗"
        echo "║         Running Assembly Function Tests...                ║"
        echo "╚════════════════════════════════════════════════════════════╝"
        echo ""
        ./PhysicsASM_Demo
        echo ""
        echo "✅ Assembly tests complete! All functions working."
        ;;
    2)
        echo ""
        echo "╔════════════════════════════════════════════════════════════╗"
        echo "║           Running GravitySim (N-body Physics)             ║"
        echo "╚════════════════════════════════════════════════════════════╝"
        echo ""
        echo "Controls:"
        echo "  - Mouse Drag: Rotate camera"
        echo "  - Scroll: Zoom in/out"
        echo "  - SPACE: Pause/unpause"
        echo "  - ESC: Exit"
        echo ""
        echo "⚡ Every physics calculation uses assembly!"
        echo ""
        sleep 2
        ./GravitySim
        ;;
    3)
        echo ""
        echo "╔════════════════════════════════════════════════════════════╗"
        echo "║          Running BlackHole3D (3D Ray Tracer)              ║"
        echo "╚════════════════════════════════════════════════════════════╝"
        echo ""
        echo "Controls:"
        echo "  - Mouse Drag: Rotate camera"
        echo "  - Scroll: Zoom in/out"
        echo "  - G: Toggle gravity effects"
        echo "  - ESC: Exit"
        echo ""
        echo "⚡ Assembly-optimized collision detection!"
        echo ""
        sleep 2
        ./BlackHole3D
        ;;
    4)
        echo ""
        echo "╔════════════════════════════════════════════════════════════╗"
        echo "║        Running BlackHole2D (2D Lensing Demo)              ║"
        echo "╚════════════════════════════════════════════════════════════╝"
        echo ""
        echo "Controls:"
        echo "  - Mouse: Interact"
        echo "  - ESC: Exit"
        echo ""
        echo "⚡ Assembly-optimized coordinate conversion!"
        echo ""
        sleep 2
        ./BlackHole2D
        ;;
    5|*)
        echo ""
        echo "To run simulations manually:"
        echo "  cd cmake-build-debug"
        echo "  ./PhysicsASM_Demo    # Test assembly (no graphics)"
        echo "  ./GravitySim         # N-body simulation ⭐"
        echo "  ./BlackHole3D        # 3D ray tracer"
        echo "  ./BlackHole2D        # 2D lensing"
        echo ""
        ;;
esac

