#!/bin/bash
# build_and_run.sh — Build all available targets and launch an interactive menu.
#
# The script detects the CPU architecture and the executables that were
# successfully built, then presents only those as menu options.
# Targets:
#   PhysicsASM_Demo      — assembly validation & benchmarks (no graphics)
#   Gravity_Grid         — N-body simulation (OpenGL 3.3)
#   BlackHole_curv       — 2-D gravitational lensing demo (OpenGL 3.3)
#   BlackHole_space_cpu  — CPU RK4 ray tracer, std::thread (OpenGL 3.3)
#   BlackHole_space_cuda — CUDA GPU ray tracer (NVIDIA only, optional)
#   BlackHole_space_metal— Metal GPU ray tracer (Apple Silicon only)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/cmake-build-debug"

ARCH=$(uname -m)   # arm64 or x86_64

# ─── Header ──────────────────────────────────────────────────────────────────
echo "╔════════════════════════════════════════════════════════════╗"
echo "║       Black Hole Assembly — Build & Run System            ║"
printf "║  Architecture: %-44s║\n" "$ARCH"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# ─── CMake configure + build ─────────────────────────────────────────────────
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "Configuring CMake..."
if ! cmake -S "${SCRIPT_DIR}" -B . -DCMAKE_BUILD_TYPE=Debug 2>&1; then
    echo ""
    echo "[ERROR] CMake configuration failed."
    exit 1
fi

echo ""
echo "Building all targets (failures on optional targets are non-fatal)..."
echo ""

# Helper: try to build a named target and print a one-line status
build_target() {
    local name="$1"
    printf "  Building %-28s... " "$name"
    if cmake --build . --target "$name" 2>/dev/null; then
        echo "OK"
    else
        echo "SKIPPED (dependencies missing or not applicable on this platform)"
    fi
}

build_target PhysicsASM_Demo
build_target Gravity_Grid
build_target BlackHole_curv
build_target BlackHole_space_cpu
build_target BlackHole_space_cuda   # no-op if CUDA not detected
build_target BlackHole_space_metal  # no-op on non-macOS-ARM64

# ─── Sanity check: the mandatory target must exist ────────────────────────────
if [[ ! -f PhysicsASM_Demo ]]; then
    echo ""
    echo "[ERROR] PhysicsASM_Demo did not build."
    echo "  Possible causes:"
    echo "    - Assembly syntax error in physics_asm_arm64.s / physics_asm.s"
    echo "    - Missing C++17 support"
    echo "  Try: cmake --build ${BUILD_DIR} --target PhysicsASM_Demo -- VERBOSE=1"
    exit 1
fi

# ─── Detect built executables ────────────────────────────────────────────────
BUILT=()
LABELS=()
CONTROLS=()

if [[ -f PhysicsASM_Demo ]]; then
    BUILT+=("PhysicsASM_Demo")
    LABELS+=("PhysicsASM_Demo   — assembly validation & benchmark (no graphics)")
    CONTROLS+=("")
fi
if [[ -f Gravity_Grid ]]; then
    BUILT+=("Gravity_Grid")
    LABELS+=("Gravity_Grid      — N-body gravitational simulation")
    CONTROLS+=("  Mouse drag: rotate  |  Scroll: zoom  |  SPACE: pause  |  ESC: quit")
fi
if [[ -f BlackHole_curv ]]; then
    BUILT+=("BlackHole_curv")
    LABELS+=("BlackHole_curv    — 2-D gravitational lensing demo")
    CONTROLS+=("  Mouse: interact  |  ESC: quit")
fi
if [[ -f BlackHole_space_cpu ]]; then
    BUILT+=("BlackHole_space_cpu")
    LABELS+=("BlackHole_space_cpu — CPU RK4 ray tracer  [${ARCH}]")
    CONTROLS+=("  Mouse drag: rotate  |  Scroll: zoom  |  G: toggle gravity  |  ESC: quit")
fi
if [[ -f BlackHole_space_cuda ]]; then
    BUILT+=("BlackHole_space_cuda")
    LABELS+=("BlackHole_space_cuda — CUDA GPU RK4 ray tracer  [NVIDIA]")
    CONTROLS+=("  Mouse drag: rotate  |  Scroll: zoom  |  G: toggle gravity  |  ESC: quit")
fi
if [[ -f BlackHole_space_metal ]]; then
    BUILT+=("BlackHole_space_metal")
    LABELS+=("BlackHole_space_metal — Metal GPU ray tracer  [Apple Silicon]")
    CONTROLS+=("  Mouse drag: rotate  |  Scroll: zoom  |  ESC: quit")
fi

# ─── Detect active GPU backend ───────────────────────────────────────────────
GPU_BACKEND="CPU only"
if [[ -f BlackHole_space_cuda ]];  then GPU_BACKEND="NVIDIA CUDA"; fi
if [[ -f BlackHole_space_metal ]]; then GPU_BACKEND="Apple Metal"; fi

# ─── Interactive menu ────────────────────────────────────────────────────────
echo ""
echo "╔════════════════════════════════════════════════════════════╗"
printf "║  GPU backend available: %-35s║\n" "$GPU_BACKEND"
echo "╠════════════════════════════════════════════════════════════╣"
echo "║  Which simulation would you like to run?                  ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

TOTAL=${#BUILT[@]}
for i in "${!LABELS[@]}"; do
    printf "  %2d) %s\n" "$((i+1))" "${LABELS[$i]}"
done
printf "  %2d) Exit\n" "$((TOTAL+1))"
echo ""
read -p "Enter choice [1-$((TOTAL+1))]: " choice

if ! [[ "$choice" =~ ^[0-9]+$ ]] || (( choice < 1 || choice > TOTAL+1 )); then
    echo "Invalid choice."
    exit 0
fi

if (( choice == TOTAL+1 )); then
    echo "Exiting."
    exit 0
fi

IDX=$((choice-1))
TARGET="${BUILT[$IDX]}"
CTRL="${CONTROLS[$IDX]}"

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
printf "║  Launching: %-47s║\n" "$TARGET"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
if [[ -n "$CTRL" ]]; then
    echo "Controls:"
    echo "$CTRL"
    echo ""
    sleep 1
fi

"./${TARGET}"

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║                    Session Complete                        ║"
echo "╚════════════════════════════════════════════════════════════╝"
