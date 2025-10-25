#!/bin/bash
# Quick launcher for Black Hole Assembly Simulations

cd "$(dirname "$0")/cmake-build-debug" || { echo "❌ Build directory not found. Run ./build_asm_demo.sh first!"; exit 1; }

echo "╔════════════════════════════════════════════════════════════╗"
echo "║     Black Hole Assembly - Simulation Launcher             ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
echo "Available simulations:"
echo ""
echo "  1) PhysicsASM_Demo  - Test assembly functions (1 second, no graphics)"
echo "  2) GravitySim       - N-body simulation (⚡ BEST for assembly showcase)"
echo "  3) BlackHole3D      - 3D black hole ray tracer"
echo "  4) BlackHole2D      - 2D gravitational lensing"
echo ""
read -p "Enter choice [1-4]: " choice

case $choice in
    1)
        echo ""
        echo "🧪 Testing Assembly Functions..."
        echo ""
        ./PhysicsASM_Demo
        ;;
    2)
        echo ""
        echo "🚀 Launching GravitySim..."
        echo ""
        echo "Controls:"
        echo "  • Mouse Drag: Rotate camera"
        echo "  • Scroll: Zoom in/out"
        echo "  • SPACE: Pause/unpause"
        echo "  • ESC: Exit"
        echo ""
        echo "⚡ Using assembly for EVERY distance, normalize, and scale operation!"
        echo ""
        sleep 2
        ./GravitySim
        ;;
    3)
        echo ""
        echo "🌌 Launching BlackHole3D..."
        echo ""
        echo "Controls:"
        echo "  • Mouse Drag: Rotate camera"
        echo "  • Scroll: Zoom in/out"
        echo "  • G: Toggle gravity"
        echo "  • ESC: Exit"
        echo ""
        echo "⚡ Using assembly for collision detection (~30K calls/frame)!"
        echo ""
        sleep 2
        ./BlackHole3D
        ;;
    4)
        echo ""
        echo "🔭 Launching BlackHole2D..."
        echo ""
        echo "Controls:"
        echo "  • Mouse: Interact"
        echo "  • ESC: Exit"
        echo ""
        echo "⚡ Using assembly for coordinate conversion!"
        echo ""
        sleep 2
        ./BlackHole2D
        ;;
    *)
        echo "❌ Invalid choice"
        exit 1
        ;;
esac

