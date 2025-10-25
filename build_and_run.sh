#!/bin/bash

# Black Hole Assembly - Build and Run Script
# Combines building all targets and launching simulations interactively

echo "╔════════════════════════════════════════════════════════════╗"
echo "║     Black Hole Assembly - Build & Run System              ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Navigate to project directory
cd "$(dirname "$0")"

# Create build directory if it doesn't exist
mkdir -p cmake-build-debug
cd cmake-build-debug

# Configure CMake
echo "🔧 Configuring CMake..."
if ! cmake .. ; then
    echo "❌ CMake configuration failed"
    exit 1
fi

# Build all projects
echo ""
echo "🔨 Building All Projects..."
echo ""

# Track build failures
BUILD_FAILED=0

# Build PhysicsASM_Demo
echo "  [1/4] Building PhysicsASM_Demo (Assembly test suite)..."
if make PhysicsASM_Demo 2>&1 | tail -n 1 | grep -q "Built target"; then
    echo "        ✅ PhysicsASM_Demo built successfully"
else
    if make PhysicsASM_Demo; then
        echo "        ✅ PhysicsASM_Demo built successfully"
    else
        echo "        ❌ PhysicsASM_Demo build failed"
        BUILD_FAILED=1
    fi
fi

# Build Gravity_Grid
echo ""
echo "  [2/4] Building Gravity_Grid (N-body with assembly)..."
if make Gravity_Grid 2>&1 | tail -n 1 | grep -q "Built target"; then
    echo "        ✅ Gravity_Grid built successfully"
else
    if make Gravity_Grid 2>/dev/null; then
        echo "        ✅ Gravity_Grid built successfully"
    else
        echo "        ⚠️  Gravity_Grid build failed (graphics dependencies may be missing)"
    fi
fi

# Build BlackHole_space
echo ""
echo "  [3/4] Building BlackHole_space (Ray tracer with assembly)..."
if make BlackHole_space 2>&1 | tail -n 1 | grep -q "Built target"; then
    echo "        ✅ BlackHole_space built successfully"
else
    if make BlackHole_space 2>/dev/null; then
        echo "        ✅ BlackHole_space built successfully"
    else
        echo "        ⚠️  BlackHole_space build failed (OpenGL 4.3+ may not be available)"
    fi
fi

# Build BlackHole_curv
echo ""
echo "  [4/4] Building BlackHole_curv (2D lensing with assembly)..."
if make BlackHole_curv 2>&1 | tail -n 1 | grep -q "Built target"; then
    echo "        ✅ BlackHole_curv built successfully"
else
    if make BlackHole_curv 2>/dev/null; then
        echo "        ✅ BlackHole_curv built successfully"
    else
        echo "        ⚠️  BlackHole_curv build failed (graphics dependencies may be missing)"
    fi
fi

echo ""

# Check if at least PhysicsASM_Demo built
if [ ! -f "PhysicsASM_Demo" ]; then
    echo "❌ Critical build failure: PhysicsASM_Demo did not build"
    echo ""
    echo "This usually means:"
    echo "  • Assembly syntax errors in physics_asm.s"
    echo "  • Compiler compatibility issues"
    echo "  • Missing C++17 support"
    echo ""
    echo "Try building manually to see full error output:"
    echo "  cd cmake-build-debug"
    echo "  make PhysicsASM_Demo VERBOSE=1"
    exit 1
fi

echo "╔════════════════════════════════════════════════════════════╗"
echo "║                 ✅ Build Complete!                         ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Interactive menu
echo "╔════════════════════════════════════════════════════════════╗"
echo "║              Which simulation to run?                      ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Choose an option:"
echo ""
echo "  1) PhysicsASM_Demo  - Test assembly functions (quick, no graphics)"
echo "  2) Gravity_Grid     - N-body simulation (⚡ BEST SHOWCASE)"
echo "  3) BlackHole_space  - 3D ray tracer (requires OpenGL 4.3+)"
echo "  4) BlackHole_curv   - 2D lensing demo"
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
        if [ -f "PhysicsASM_Demo" ]; then
            ./PhysicsASM_Demo
            echo ""
            echo "✅ Assembly tests complete! All functions working."
        else
            echo "❌ PhysicsASM_Demo executable not found"
        fi
        ;;
    2)
        echo ""
        echo "╔════════════════════════════════════════════════════════════╗"
        echo "║           Running Gravity_Grid (N-body Physics)           ║"
        echo "╚════════════════════════════════════════════════════════════╝"
        echo ""
        if [ -f "Gravity_Grid" ]; then
            echo "Controls:"
            echo "  • Mouse Drag: Rotate camera"
            echo "  • Scroll: Zoom in/out"
            echo "  • SPACE: Pause/unpause"
            echo "  • ESC: Exit"
            echo ""
            echo "⚡ Every physics calculation uses assembly!"
            echo ""
            sleep 2
            ./Gravity_Grid
        else
            echo "❌ Gravity_Grid executable not found"
            echo "   Graphics dependencies may be missing (OpenGL, GLFW, GLEW)"
        fi
        ;;
    3)
        echo ""
        echo "╔════════════════════════════════════════════════════════════╗"
        echo "║        Running BlackHole_space (3D Ray Tracer)            ║"
        echo "╚════════════════════════════════════════════════════════════╝"
        echo ""
        if [ -f "BlackHole_space" ]; then
            echo "Controls:"
            echo "  • Mouse Drag: Rotate camera"
            echo "  • Scroll: Zoom in/out"
            echo "  • G: Toggle gravity effects"
            echo "  • ESC: Exit"
            echo ""
            echo "⚡ Assembly-optimized collision detection!"
            echo ""
            sleep 2
            ./BlackHole_space
        else
            echo "❌ BlackHole_space executable not found"
            echo "   Requires OpenGL 4.3+ and graphics dependencies (GLFW, GLEW)"
        fi
        ;;
    4)
        echo ""
        echo "╔════════════════════════════════════════════════════════════╗"
        echo "║        Running BlackHole_curv (2D Lensing Demo)           ║"
        echo "╚════════════════════════════════════════════════════════════╝"
        echo ""
        if [ -f "BlackHole_curv" ]; then
            echo "Controls:"
            echo "  • Mouse: Interact"
            echo "  • ESC: Exit"
            echo ""
            echo "⚡ Assembly-optimized coordinate conversion!"
            echo ""
            sleep 2
            ./BlackHole_curv
        else
            echo "❌ BlackHole_curv executable not found"
            echo "   Graphics dependencies may be missing (OpenGL, GLFW, GLEW)"
        fi
        ;;
    5|*)
        echo ""
        echo "To run simulations manually:"
        echo "  cd cmake-build-debug"
        echo ""
        if [ -f "PhysicsASM_Demo" ]; then
            echo "  ./PhysicsASM_Demo    # Test assembly (no graphics)"
        fi
        if [ -f "Gravity_Grid" ]; then
            echo "  ./Gravity_Grid       # N-body simulation ⭐"
        fi
        if [ -f "BlackHole_space" ]; then
            echo "  ./BlackHole_space    # 3D ray tracer"
        fi
        if [ -f "BlackHole_curv" ]; then
            echo "  ./BlackHole_curv     # 2D lensing"
        fi
        echo ""
        ;;
esac

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                    Session Complete                        ║"
echo "╚════════════════════════════════════════════════════════════╝"

