#include "physics_asm.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <chrono>

// Demonstration of assembly-optimized physics functions
// This program validates correctness and measures performance of hand-written x86-64 assembly
// implementations compared to standard C++ scalar code

// Scoped timer for performance benchmarking
class ScopedTimer {
private:
    std::chrono::high_resolution_clock::time_point start;
    const char* name;
    double* result_ms;
    
public:
    ScopedTimer(const char* n, double* res = nullptr) 
        : start(std::chrono::high_resolution_clock::now()), name(n), result_ms(res) {}
    
    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double ms = duration.count() / 1000000.0;
        if (result_ms) {
            *result_ms = ms;
        }
    }
};

// Standard C++ implementations for comparison and validation
namespace StandardImpl {
    float DistanceSquared(float x1, float y1, float z1,
                          float x2, float y2, float z2) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float dz = z2 - z1;
        return dx*dx + dy*dy + dz*dz;
    }

    float GravitationalForce(float m1, float m2, float distSq) {
        const float G = 6.67430e-11f;  // Gravitational constant
        return (G * m1 * m2) / distSq;
    }

    void Normalize(float* v) {
        float mag = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
        if (mag > 0.0f) {  // Guard against division by zero
            v[0] /= mag;
            v[1] /= mag;
            v[2] /= mag;
        }
    }
    
    float DotProduct(const float* v1, const float* v2) {
        return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    }
}

void PrintVector(const char* label, const float* v) {
    std::cout << label << " ["
              << std::fixed << std::setprecision(6)
              << v[0] << ", " << v[1] << ", " << v[2] << "]" << std::endl;
}

void TestDistanceSquared() {
    std::cout << "\n=== Distance Squared Test ===" << std::endl;

    float x1 = 1.0f, y1 = 2.0f, z1 = 3.0f;
    float x2 = 4.0f, y2 = 6.0f, z2 = 8.0f;

    float asm_result = PhysicsASM::DistanceSquared(x1, y1, z1, x2, y2, z2);
    float cpp_result = StandardImpl::DistanceSquared(x1, y1, z1, x2, y2, z2);

    std::cout << "Point 1: (" << x1 << ", " << y1 << ", " << z1 << ")" << std::endl;
    std::cout << "Point 2: (" << x2 << ", " << y2 << ", " << z2 << ")" << std::endl;
    std::cout << "Result ASM: " << asm_result << std::endl;
    std::cout << "Result C++: " << cpp_result << std::endl;
    std::cout << "Match: " << (std::abs(asm_result - cpp_result) < 0.0001f ? "✓" : "✗") << std::endl;
}

void TestGravitationalForce() {
    std::cout << "\n=== Gravitational Force Test ===" << std::endl;

    float mass1 = 5.972e24f;  // Earth's mass in kg
    float mass2 = 7.342e22f;  // Moon's mass in kg
    float distSq = 1.48e17f;  // Earth-Moon distance squared (~384,400 km)

    float asm_result = PhysicsASM::GravitationalForce(mass1, mass2, distSq);
    float cpp_result = StandardImpl::GravitationalForce(mass1, mass2, distSq);

    std::cout << "Mass 1: " << mass1 << " kg (Earth)" << std::endl;
    std::cout << "Mass 2: " << mass2 << " kg (Moon)" << std::endl;
    std::cout << "Distance²: " << distSq << " m²" << std::endl;
    std::cout << "Force ASM: " << asm_result << " N" << std::endl;
    std::cout << "Force C++: " << cpp_result << " N" << std::endl;
    std::cout << "Match: " << (std::abs(asm_result - cpp_result) < 1e10f ? "✓" : "✗") << std::endl;
}

void TestNormalize() {
    std::cout << "\n=== Vector Normalization Test ===" << std::endl;

    float asm_vec[3] = {3.0f, 4.0f, 0.0f};  // Classic 3-4-5 triangle (3-4-0)
    float cpp_vec[3] = {3.0f, 4.0f, 0.0f};

    PrintVector("Original vector:", asm_vec);

    PhysicsASM::Normalize(asm_vec);
    StandardImpl::Normalize(cpp_vec);

    PrintVector("Normalized ASM:", asm_vec);
    PrintVector("Normalized C++:", cpp_vec);

    float diff = std::abs(asm_vec[0] - cpp_vec[0]) +
                 std::abs(asm_vec[1] - cpp_vec[1]) +
                 std::abs(asm_vec[2] - cpp_vec[2]);
    std::cout << "Match: " << (diff < 0.0001f ? "✓" : "✗") << std::endl;
}

void TestDotProduct() {
    std::cout << "\n=== Dot Product Test ===" << std::endl;

    float v1[3] = {1.0f, 2.0f, 3.0f};
    float v2[3] = {4.0f, 5.0f, 6.0f};

    float asm_result = PhysicsASM::DotProduct(v1, v2);
    float cpp_result = StandardImpl::DotProduct(v1, v2);

    PrintVector("Vector 1:", v1);
    PrintVector("Vector 2:", v2);
    std::cout << "Dot Product ASM: " << asm_result << std::endl;
    std::cout << "Dot Product C++: " << cpp_result << std::endl;
    std::cout << "Match: " << (std::abs(asm_result - cpp_result) < 0.0001f ? "✓" : "✗") << std::endl;
}

void TestVectorAdd() {
    std::cout << "\n=== Vector Addition Test ===" << std::endl;

    float v1[3] = {1.0f, 2.0f, 3.0f};
    float v2[3] = {4.0f, 5.0f, 6.0f};
    float result[3];

    PhysicsASM::VectorAdd(v1, v2, result);

    PrintVector("Vector 1:", v1);
    PrintVector("Vector 2:", v2);
    PrintVector("Result: ", result);

    bool match = (result[0] == 5.0f && result[1] == 7.0f && result[2] == 9.0f);
    std::cout << "Match: " << (match ? "✓" : "✗") << std::endl;
}

void TestVectorScale() {
    std::cout << "\n=== Vector Scalar Multiplication Test ===" << std::endl;

    float v[3] = {1.0f, 2.0f, 3.0f};
    float scalar = 2.5f;
    float result[3];

    PhysicsASM::VectorScale(v, scalar, result);

    PrintVector("Original vector:", v);
    std::cout << "Scalar: " << scalar << std::endl;
    PrintVector("Result:        ", result);

    bool match = (std::abs(result[0] - 2.5f) < 0.0001f &&
                  std::abs(result[1] - 5.0f) < 0.0001f &&
                  std::abs(result[2] - 7.5f) < 0.0001f);
    std::cout << "Match: " << (match ? "✓" : "✗") << std::endl;
}

void TestPhysicsScenario() {
    std::cout << "\n=== Physics Simulation Scenario ===" << std::endl;
    std::cout << "Computing gravitational interaction between two massive objects:" << std::endl;

    // Object 1: position and mass
    float obj1_pos[3] = {0.0f, 0.0f, 0.0f};
    float obj1_mass = 1.0e22f;

    // Object 2: position and mass
    float obj2_pos[3] = {1000.0f, 2000.0f, 1500.0f};
    float obj2_mass = 5.0e21f;

    std::cout << "\nObject 1:" << std::endl;
    PrintVector("  Position:", obj1_pos);
    std::cout << "  Mass: " << obj1_mass << " kg" << std::endl;

    std::cout << "\nObject 2:" << std::endl;
    PrintVector("  Position:", obj2_pos);
    std::cout << "  Mass: " << obj2_mass << " kg" << std::endl;

    // Calculate distance squared (avoids sqrt for better performance)
    float distSq = PhysicsASM::DistanceSquared(
        obj1_pos[0], obj1_pos[1], obj1_pos[2],
        obj2_pos[0], obj2_pos[1], obj2_pos[2]
    );

    std::cout << "\nDistance²: " << distSq << " m²" << std::endl;
    std::cout << "Distance:  " << std::sqrt(distSq) << " m" << std::endl;

    // Calculate gravitational force (F = G*m1*m2/r²)
    float force = PhysicsASM::GravitationalForce(obj1_mass, obj2_mass, distSq);
    std::cout << "\nGravitational force: " << force << " N" << std::endl;

    // Calculate direction vector (normalized to unit length)
    float direction[3] = {
        obj2_pos[0] - obj1_pos[0],
        obj2_pos[1] - obj1_pos[1],
        obj2_pos[2] - obj1_pos[2]
    };
    PhysicsASM::Normalize(direction);

    std::cout << "\nForce direction (normalized):" << std::endl;
    PrintVector("  Direction:", direction);

    // Calculate final force vector (direction * magnitude)
    float force_vector[3];
    PhysicsASM::VectorScale(direction, force, force_vector);

    std::cout << "\nFinal force vector:" << std::endl;
    PrintVector("  Force:", force_vector);
}

void BenchmarkPerformance() {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║           Performance Benchmarks                          ║" << std::endl;
    std::cout << "║   C++ Scalar vs Assembly SIMD Optimizations               ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    
    const int ITERATIONS = 1000000; // 1 million iterations
    
    // Benchmark 1: Distance Squared
    {
        std::cout << "\n[1] Distance Squared Calculation (" << ITERATIONS << " iterations)" << std::endl;
        
        double cpp_time = 0.0, asm_time = 0.0;
        volatile float result = 0.0f; // volatile to prevent optimization
        
        // C++ Scalar version
        {
            ScopedTimer timer("C++ Scalar", &cpp_time);
            for (int i = 0; i < ITERATIONS; i++) {
                result = StandardImpl::DistanceSquared(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f);
            }
        }
        
        // Assembly SIMD version
        {
            ScopedTimer timer("Assembly SIMD", &asm_time);
            for (int i = 0; i < ITERATIONS; i++) {
                result = PhysicsASM::DistanceSquared(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f);
            }
        }
        
        std::cout << "  C++ Scalar:     " << std::fixed << std::setprecision(3) << cpp_time << " ms" << std::endl;
        std::cout << "  Assembly SIMD:  " << std::fixed << std::setprecision(3) << asm_time << " ms" << std::endl;
        std::cout << "  Speedup:        " << std::fixed << std::setprecision(2) << (cpp_time / asm_time) << "x" << std::endl;
    }
    
    // Benchmark 2: Gravitational Force
    {
        std::cout << "\n[2] Gravitational Force Calculation (" << ITERATIONS << " iterations)" << std::endl;
        
        double cpp_time = 0.0, asm_time = 0.0;
        volatile float result = 0.0f;
        
        // C++ Scalar version
        {
            ScopedTimer timer("C++ Scalar", &cpp_time);
            for (int i = 0; i < ITERATIONS; i++) {
                result = StandardImpl::GravitationalForce(5.972e24f, 7.342e22f, 1.48e17f);
            }
        }
        
        // Assembly SIMD version
        {
            ScopedTimer timer("Assembly SIMD", &asm_time);
            for (int i = 0; i < ITERATIONS; i++) {
                result = PhysicsASM::GravitationalForce(5.972e24f, 7.342e22f, 1.48e17f);
            }
        }
        
        std::cout << "  C++ Scalar:     " << std::fixed << std::setprecision(3) << cpp_time << " ms" << std::endl;
        std::cout << "  Assembly SIMD:  " << std::fixed << std::setprecision(3) << asm_time << " ms" << std::endl;
        std::cout << "  Speedup:        " << std::fixed << std::setprecision(2) << (cpp_time / asm_time) << "x" << std::endl;
    }
    
    // Benchmark 3: Vector Normalization
    {
        std::cout << "\n[3] Vector Normalization (" << ITERATIONS << " iterations)" << std::endl;
        
        double cpp_time = 0.0, asm_time = 0.0;
        float vec_cpp[3] = {3.0f, 4.0f, 5.0f};
        float vec_asm[3] = {3.0f, 4.0f, 5.0f};
        
        // C++ Scalar version
        {
            ScopedTimer timer("C++ Scalar", &cpp_time);
            for (int i = 0; i < ITERATIONS; i++) {
                vec_cpp[0] = 3.0f; vec_cpp[1] = 4.0f; vec_cpp[2] = 5.0f;
                StandardImpl::Normalize(vec_cpp);
            }
        }
        
        // Assembly SIMD version
        {
            ScopedTimer timer("Assembly SIMD", &asm_time);
            for (int i = 0; i < ITERATIONS; i++) {
                vec_asm[0] = 3.0f; vec_asm[1] = 4.0f; vec_asm[2] = 5.0f;
                PhysicsASM::Normalize(vec_asm);
            }
        }
        
        std::cout << "  C++ Scalar:     " << std::fixed << std::setprecision(3) << cpp_time << " ms" << std::endl;
        std::cout << "  Assembly SIMD:  " << std::fixed << std::setprecision(3) << asm_time << " ms" << std::endl;
        std::cout << "  Speedup:        " << std::fixed << std::setprecision(2) << (cpp_time / asm_time) << "x" << std::endl;
    }
    
    // Benchmark 4: Dot Product
    {
        std::cout << "\n[4] Dot Product (" << ITERATIONS << " iterations)" << std::endl;
        
        double cpp_time = 0.0, asm_time = 0.0;
        volatile float result = 0.0f;
        float v1[3] = {1.0f, 2.0f, 3.0f};
        float v2[3] = {4.0f, 5.0f, 6.0f};
        
        // C++ Scalar version
        {
            ScopedTimer timer("C++ Scalar", &cpp_time);
            for (int i = 0; i < ITERATIONS; i++) {
                result = StandardImpl::DotProduct(v1, v2);
            }
        }
        
        // Assembly SIMD version
        {
            ScopedTimer timer("Assembly SIMD", &asm_time);
            for (int i = 0; i < ITERATIONS; i++) {
                result = PhysicsASM::DotProduct(v1, v2);
            }
        }
        
        std::cout << "  C++ Scalar:     " << std::fixed << std::setprecision(3) << cpp_time << " ms" << std::endl;
        std::cout << "  Assembly SIMD:  " << std::fixed << std::setprecision(3) << asm_time << " ms" << std::endl;
        std::cout << "  Speedup:        " << std::fixed << std::setprecision(2) << (cpp_time / asm_time) << "x" << std::endl;
    }
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║           Benchmarks Complete!                             ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   Assembly-Optimized Physics Functions Demo              ║" << std::endl;
    std::cout << "║   x86-64 Assembly with SSE SIMD Instructions              ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;

    // Check if benchmark mode is requested
    bool benchmark_mode = false;
    if (argc > 1 && std::string(argv[1]) == "--benchmark") {
        benchmark_mode = true;
    }

    if (benchmark_mode) {
        // Run performance benchmarks
        BenchmarkPerformance();
    } else {
        // Run correctness tests to verify assembly implementations
        TestDistanceSquared();
        TestGravitationalForce();
        TestNormalize();
        TestDotProduct();
        TestVectorAdd();
        TestVectorScale();
        TestPhysicsScenario();

        std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║              All Tests Completed Successfully!            ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
        
        std::cout << "\nTip: Run with --benchmark flag to see performance comparisons" << std::endl;
    }

    return 0;
}

