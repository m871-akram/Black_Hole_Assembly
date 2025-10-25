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
echo "Available executables:"
echo "  1. ./PhysicsASM_Demo  - Assembly function tests (no graphics)"
echo "  2. ./GravitySim       - N-body simulation (⚡ HEAVY assembly usage)"
echo "  3. ./BlackHole3D      - 3D ray tracer (⚡ assembly collision detection)"
echo "  4. ./BlackHole2D      - 2D lensing demo (⚡ assembly coordinates)"
echo ""
echo "Would you like to run PhysicsASM_Demo to test assembly functions?"
read -p "(y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║         Running Assembly Function Tests...                ║"
    echo "╚════════════════════════════════════════════════════════════╝"
    echo ""
    ./PhysicsASM_Demo
    echo ""
    echo "╔════════════════════════════════════════════════════════════╗"
    echo "║  Assembly functions verified! Ready for simulations!       ║"
    echo "╚════════════════════════════════════════════════════════════╝"
fi

