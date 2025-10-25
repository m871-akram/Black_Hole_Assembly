# 🚀 Quick Start Guide - Black Hole Assembly Project

## ✅ Build Everything

```bash
./build_asm_demo.sh
```

This builds all 4 executables:
1. **PhysicsASM_Demo** - Assembly test suite
2. **GravitySim** - N-body simulation with assembly
3. **BlackHole3D** - 3D ray tracer with assembly
4. **BlackHole2D** - 2D lensing with assembly

---

## 🎮 Running the Simulations

### Option 1: PhysicsASM_Demo (No Graphics)
```bash
cd cmake-build-debug
./PhysicsASM_Demo
```
**What it does:** Tests all 6 assembly functions, shows results  
**Requirements:** None (no OpenGL needed)  
**Duration:** ~1 second  
**Assembly usage:** 100% (all functions tested)

---

### Option 2: GravitySim (⚡ MOST Assembly Usage)
```bash
cd cmake-build-debug
./GravitySim
```
**What it does:** Real-time N-body gravity simulation  
**Requirements:** OpenGL, GLEW, GLFW, GLM  
**Controls:**
- Mouse drag: Rotate camera
- Scroll: Zoom in/out  
- SPACE: Pause/unpause
- ESC: Exit

**Assembly functions used:**
- ✅ `DistanceSquared` - Every object pair calculation
- ✅ `Normalize` - Every direction vector
- ✅ `VectorScale` - Every force application

**Performance:** Thousands of assembly calls per frame!

---

### Option 3: BlackHole3D (⚡ High Assembly Usage)
```bash
cd cmake-build-debug
./BlackHole3D
```
**What it does:** 3D black hole ray tracing with gravitational lensing  
**Requirements:** OpenGL 4.3+, Compute Shaders, GLEW, GLFW, GLM  
**Controls:**
- Mouse drag: Rotate camera
- Scroll: Zoom in/out
- G: Toggle gravity effects
- ESC: Exit

**Assembly functions used:**
- ✅ `DistanceSquared` - Black hole collision detection

**Performance:** ~30,000 assembly calls per frame at default resolution!

---

### Option 4: BlackHole2D (⚡ Medium Assembly Usage)
```bash
cd cmake-build-debug
./BlackHole2D
```
**What it does:** 2D gravitational lensing visualization  
**Requirements:** OpenGL, GLEW, GLFW, GLM  
**Controls:**
- Mouse: Interact
- ESC: Exit

**Assembly functions used:**
- ✅ `DistanceSquared` - Polar coordinate conversion

**Performance:** Hundreds of assembly calls per second

---

## 📊 Assembly Usage Comparison

| Simulation | Assembly Calls/Frame | Assembly Functions | Difficulty |
|------------|---------------------|-------------------|------------|
| **GravitySim** | 1,000s - 10,000s | 3 types | ⭐⭐⭐⭐⭐ |
| **BlackHole3D** | ~30,000 | 1 type | ⭐⭐⭐⭐ |
| **BlackHole2D** | 100s | 1 type | ⭐⭐⭐ |
| **PhysicsASM_Demo** | Test only | All 6 | ⭐ |

---

## 🔍 Verifying Assembly Integration

### Check if executables were built:
```bash
cd cmake-build-debug
ls -lh PhysicsASM_Demo GravitySim BlackHole3D BlackHole2D
```

### Verify assembly code is linked:
```bash
nm -g GravitySim | grep vector_distance_squared
# Should show: vector_distance_squared (from assembly)
```

### Run assembly tests:
```bash
./PhysicsASM_Demo
# Should show: 6/6 tests passing ✓
```

---

## 🐛 Troubleshooting

### "Command not found"
```bash
chmod +x build_asm_demo.sh
./build_asm_demo.sh
```

### "OpenGL not found" (graphics demos)
Install dependencies:
```bash
# Ubuntu/Debian
sudo apt-get install libglew-dev libglfw3-dev libglm-dev

# macOS
brew install glew glfw glm
```

### "PhysicsASM_Demo works but graphics don't"
That's OK! PhysicsASM_Demo doesn't need graphics libraries.
The assembly code is still integrated, you just can't run the visual simulations.

### Build errors
```bash
cd cmake-build-debug
make clean
cmake ..
make VERBOSE=1
```

---

## 📈 Performance Tips

### For best performance in GravitySim:
1. Start with fewer objects
2. Use fullscreen mode
3. Modern CPU with SSE support
4. Release build: `cmake -DCMAKE_BUILD_TYPE=Release ..`

### For best performance in BlackHole3D:
1. Lower resolution if needed
2. Modern GPU with OpenGL 4.3+
3. Compute shader support required

---

## 🎯 Quick Test Workflow

```bash
# 1. Build everything
./build_asm_demo.sh

# 2. Test assembly functions (fast)
cd cmake-build-debug
./PhysicsASM_Demo
# Should show: All tests passing ✓

# 3. Try GravitySim (most assembly usage)
./GravitySim
# Watch the N-body simulation with assembly-optimized physics!

# 4. (Optional) Try other simulations
./BlackHole3D  # If you have OpenGL 4.3+
./BlackHole2D  # 2D version
```

---

## 📝 What Each Executable Shows

### PhysicsASM_Demo Output:
```
✓ Distance Squared: 50 = 50
✓ Gravitational Force: 1.97733e+20 = 1.97733e+20
✓ Vector Normalization: [0.6, 0.8, 0.0]
✓ Dot Product: 32 = 32
✓ Vector Addition: [5, 7, 9]
✓ Vector Scaling: [2.5, 5.0, 7.5]
```

### GravitySim Shows:
- Multiple colored spheres (planets)
- Gravitational grid warping
- Real-time orbital mechanics
- **ALL using assembly for distance/force calculations!**

### BlackHole3D Shows:
- 3D black hole with event horizon
- Gravitational lensing effects
- Stars and accretion disk
- **Assembly-optimized collision detection!**

### BlackHole2D Shows:
- 2D visualization of spacetime curvature
- Light rays bending around black hole
- **Assembly-optimized coordinate conversion!**

---

## 🏆 Success Criteria

✅ **PhysicsASM_Demo shows 6/6 tests passing**  
✅ **GravitySim runs with smooth physics**  
✅ **Assembly code is used in all simulations**  
✅ **No crashes or errors**

---

## 💡 Pro Tips

1. **Always test PhysicsASM_Demo first** - it validates assembly works
2. **GravitySim is the best showcase** - most assembly usage
3. **Use SPACE in GravitySim** - pause to inspect physics state
4. **BlackHole3D needs good GPU** - falls back to CPU otherwise
5. **Check FPS** - assembly should improve performance

---

## 📚 More Information

- **ASSEMBLY_INTEGRATION.md** - Detailed integration info
- **FINAL_SUCCESS.md** - Test results and status
- **README.md** - Full project documentation
- **INTEGRATION_GUIDE.md** - How to add more assembly

---

**🚀 Ready to showcase your assembly-optimized physics simulations! 🚀**

Build with: `./build_asm_demo.sh`  
Test with: `./cmake-build-debug/PhysicsASM_Demo`  
Run with: `./cmake-build-debug/GravitySim`

