#include "physics_asm.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>

// ============================================================================
// Assembly Functions Demonstration and Performance Test
// ============================================================================
// This file demonstrates the usage of assembly-optimized physics functions
// and compares them with standard C++ implementations for verification.
// ============================================================================

// Standard C++ implementations for comparison
namespace StandardImpl {
    float DistanceSquared(float x1, float y1, float z1,
                          float x2, float y2, float z2) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float dz = z2 - z1;
        return dx*dx + dy*dy + dz*dz;
    }

    float GravitationalForce(float m1, float m2, float distSq) {
        const float G = 6.67430e-11f;
        return (G * m1 * m2) / distSq;
    }

    void Normalize(float* v) {
        float mag = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
        if (mag > 0.0f) {
            v[0] /= mag;
            v[1] /= mag;
            v[2] /= mag;
        }
    }
}

void PrintVector(const char* label, const float* v) {
    std::cout << label << " ["
              << std::fixed << std::setprecision(6)
              << v[0] << ", " << v[1] << ", " << v[2] << "]" << std::endl;
}

void TestDistanceSquared() {
    std::cout << "\n=== Testing Distance Squared ===" << std::endl;

    float x1 = 1.0f, y1 = 2.0f, z1 = 3.0f;
    float x2 = 4.0f, y2 = 6.0f, z2 = 8.0f;

    float asm_result = PhysicsASM::DistanceSquared(x1, y1, z1, x2, y2, z2);
    float cpp_result = StandardImpl::DistanceSquared(x1, y1, z1, x2, y2, z2);

    std::cout << "Point 1: (" << x1 << ", " << y1 << ", " << z1 << ")" << std::endl;
    std::cout << "Point 2: (" << x2 << ", " << y2 << ", " << z2 << ")" << std::endl;
    std::cout << "Assembly result: " << asm_result << std::endl;
    std::cout << "C++ result:      " << cpp_result << std::endl;
    std::cout << "Match: " << (std::abs(asm_result - cpp_result) < 0.0001f ? "✓" : "✗") << std::endl;
}

void TestGravitationalForce() {
    std::cout << "\n=== Testing Gravitational Force ===" << std::endl;

    float mass1 = 5.972e24f;  // Earth mass (kg)
    float mass2 = 7.342e22f;  // Moon mass (kg)
    float distSq = 1.48e17f;  // ~384,400 km squared

    float asm_result = PhysicsASM::GravitationalForce(mass1, mass2, distSq);
    float cpp_result = StandardImpl::GravitationalForce(mass1, mass2, distSq);

    std::cout << "Mass 1: " << mass1 << " kg (Earth)" << std::endl;
    std::cout << "Mass 2: " << mass2 << " kg (Moon)" << std::endl;
    std::cout << "Distance²: " << distSq << " m²" << std::endl;
    std::cout << "Assembly force: " << asm_result << " N" << std::endl;
    std::cout << "C++ force:      " << cpp_result << " N" << std::endl;
    std::cout << "Match: " << (std::abs(asm_result - cpp_result) < 1e10f ? "✓" : "✗") << std::endl;
}

void TestNormalize() {
    std::cout << "\n=== Testing Vector Normalization ===" << std::endl;

    float asm_vec[3] = {3.0f, 4.0f, 0.0f};
    float cpp_vec[3] = {3.0f, 4.0f, 0.0f};

    PrintVector("Original vector:", asm_vec);

    PhysicsASM::Normalize(asm_vec);
    StandardImpl::Normalize(cpp_vec);

    PrintVector("Assembly normalized:", asm_vec);
    PrintVector("C++ normalized:     ", cpp_vec);

    float diff = std::abs(asm_vec[0] - cpp_vec[0]) +
                 std::abs(asm_vec[1] - cpp_vec[1]) +
                 std::abs(asm_vec[2] - cpp_vec[2]);
    std::cout << "Match: " << (diff < 0.0001f ? "✓" : "✗") << std::endl;
}

void TestDotProduct() {
    std::cout << "\n=== Testing Dot Product ===" << std::endl;

    float v1[3] = {1.0f, 2.0f, 3.0f};
    float v2[3] = {4.0f, 5.0f, 6.0f};

    float asm_result = PhysicsASM::DotProduct(v1, v2);
    float cpp_result = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];

    PrintVector("Vector 1:", v1);
    PrintVector("Vector 2:", v2);
    std::cout << "Assembly dot product: " << asm_result << std::endl;
    std::cout << "C++ dot product:      " << cpp_result << std::endl;
    std::cout << "Match: " << (std::abs(asm_result - cpp_result) < 0.0001f ? "✓" : "✗") << std::endl;
}

void TestVectorAdd() {
    std::cout << "\n=== Testing Vector Addition ===" << std::endl;

    float v1[3] = {1.0f, 2.0f, 3.0f};
    float v2[3] = {4.0f, 5.0f, 6.0f};
    float result[3];

    PhysicsASM::VectorAdd(v1, v2, result);

    PrintVector("Vector 1:", v1);
    PrintVector("Vector 2:", v2);
    PrintVector("Result:   ", result);

    bool match = (result[0] == 5.0f && result[1] == 7.0f && result[2] == 9.0f);
    std::cout << "Match: " << (match ? "✓" : "✗") << std::endl;
}

void TestVectorScale() {
    std::cout << "\n=== Testing Vector Scaling ===" << std::endl;

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
    std::cout << "Calculating gravitational interaction between two objects:" << std::endl;

    // Object 1: Position and mass
    float obj1_pos[3] = {0.0f, 0.0f, 0.0f};
    float obj1_mass = 1.0e22f;

    // Object 2: Position and mass
    float obj2_pos[3] = {1000.0f, 2000.0f, 1500.0f};
    float obj2_mass = 5.0e21f;

    std::cout << "\nObject 1:" << std::endl;
    PrintVector("  Position:", obj1_pos);
    std::cout << "  Mass: " << obj1_mass << " kg" << std::endl;

    std::cout << "\nObject 2:" << std::endl;
    PrintVector("  Position:", obj2_pos);
    std::cout << "  Mass: " << obj2_mass << " kg" << std::endl;

    // Calculate distance squared
    float distSq = PhysicsASM::DistanceSquared(
        obj1_pos[0], obj1_pos[1], obj1_pos[2],
        obj2_pos[0], obj2_pos[1], obj2_pos[2]
    );

    std::cout << "\nDistance²: " << distSq << " m²" << std::endl;
    std::cout << "Distance:  " << std::sqrt(distSq) << " m" << std::endl;

    // Calculate gravitational force
    float force = PhysicsASM::GravitationalForce(obj1_mass, obj2_mass, distSq);
    std::cout << "\nGravitational force: " << force << " N" << std::endl;

    // Calculate direction vector (normalized)
    float direction[3] = {
        obj2_pos[0] - obj1_pos[0],
        obj2_pos[1] - obj1_pos[1],
        obj2_pos[2] - obj1_pos[2]
    };
    PhysicsASM::Normalize(direction);

    std::cout << "\nForce direction (normalized):" << std::endl;
    PrintVector("  Direction:", direction);

    // Calculate force vector
    float force_vector[3];
    PhysicsASM::VectorScale(direction, force, force_vector);

    std::cout << "\nForce vector:" << std::endl;
    PrintVector("  Force:", force_vector);
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   Assembly-Optimized Physics Functions - Demo & Test      ║" << std::endl;
    std::cout << "║   Author: Mohammed Akram Lrhorfi                           ║" << std::endl;
    std::cout << "║   Architecture: x86-64 with SSE Instructions               ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;

    TestDistanceSquared();
    TestGravitationalForce();
    TestNormalize();
    TestDotProduct();
    TestVectorAdd();
    TestVectorScale();
    TestPhysicsScenario();

    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    All Tests Complete!                     ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;

    return 0;
}

