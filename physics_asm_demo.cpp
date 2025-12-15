#include "physics_asm.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <chrono>

// Démo des fonctions assembleur pour les calculs physiques
// En gros c'est pour tester que tout marche bien et comparer avec le C++ standard
// Si ça match pas y'a un problème dans l'ASM mdr

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

// Implémentations standard en C++ pour comparer (genre les versions lentes quoi)
namespace StandardImpl {
    float DistanceSquared(float x1, float y1, float z1,
                          float x2, float y2, float z2) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        float dz = z2 - z1;
        return dx*dx + dy*dy + dz*dz;
    }

    float GravitationalForce(float m1, float m2, float distSq) {
        const float G = 6.67430e-11f;  // constante gravitationnelle, ultra importante
        return (G * m1 * m2) / distSq;
    }

    void Normalize(float* v) {
        float mag = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
        if (mag > 0.0f) {  // faut check sinon on divise par 0 et ça crash
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
    std::cout << "\n=== Test de Distance au Carré ===" << std::endl;

    float x1 = 1.0f, y1 = 2.0f, z1 = 3.0f;
    float x2 = 4.0f, y2 = 6.0f, z2 = 8.0f;

    float asm_result = PhysicsASM::DistanceSquared(x1, y1, z1, x2, y2, z2);
    float cpp_result = StandardImpl::DistanceSquared(x1, y1, z1, x2, y2, z2);

    std::cout << "Point 1: (" << x1 << ", " << y1 << ", " << z1 << ")" << std::endl;
    std::cout << "Point 2: (" << x2 << ", " << y2 << ", " << z2 << ")" << std::endl;
    std::cout << "Résultat ASM: " << asm_result << std::endl;
    std::cout << "Résultat C++: " << cpp_result << std::endl;
    std::cout << "Match: " << (std::abs(asm_result - cpp_result) < 0.0001f ? "✓" : "✗") << std::endl;
}

void TestGravitationalForce() {
    std::cout << "\n=== Test de Force Gravitationnelle ===" << std::endl;

    float mass1 = 5.972e24f;  // masse de la Terre en kg
    float mass2 = 7.342e22f;  // masse de la Lune en kg
    float distSq = 1.48e17f;  // distance Terre-Lune au carré (~384,400 km²)

    float asm_result = PhysicsASM::GravitationalForce(mass1, mass2, distSq);
    float cpp_result = StandardImpl::GravitationalForce(mass1, mass2, distSq);

    std::cout << "Masse 1: " << mass1 << " kg (Terre)" << std::endl;
    std::cout << "Masse 2: " << mass2 << " kg (Lune)" << std::endl;
    std::cout << "Distance²: " << distSq << " m²" << std::endl;
    std::cout << "Force ASM: " << asm_result << " N" << std::endl;
    std::cout << "Force C++: " << cpp_result << " N" << std::endl;
    std::cout << "Match: " << (std::abs(asm_result - cpp_result) < 1e10f ? "✓" : "✗") << std::endl;
}

void TestNormalize() {
    std::cout << "\n=== Test de Normalisation de Vecteur ===" << std::endl;

    float asm_vec[3] = {3.0f, 4.0f, 0.0f};  // vecteur classique 3-4-5 (enfin 3-4-0)
    float cpp_vec[3] = {3.0f, 4.0f, 0.0f};

    PrintVector("Vecteur original:", asm_vec);

    PhysicsASM::Normalize(asm_vec);
    StandardImpl::Normalize(cpp_vec);

    PrintVector("Normalisé ASM:", asm_vec);
    PrintVector("Normalisé C++:", cpp_vec);

    float diff = std::abs(asm_vec[0] - cpp_vec[0]) +
                 std::abs(asm_vec[1] - cpp_vec[1]) +
                 std::abs(asm_vec[2] - cpp_vec[2]);
    std::cout << "Match: " << (diff < 0.0001f ? "✓" : "✗") << std::endl;
}

void TestDotProduct() {
    std::cout << "\n=== Test de Produit Scalaire ===" << std::endl;

    float v1[3] = {1.0f, 2.0f, 3.0f};
    float v2[3] = {4.0f, 5.0f, 6.0f};

    float asm_result = PhysicsASM::DotProduct(v1, v2);
    float cpp_result = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];  // calcul à la main

    PrintVector("Vecteur 1:", v1);
    PrintVector("Vecteur 2:", v2);
    std::cout << "Produit scalaire ASM: " << asm_result << std::endl;
    std::cout << "Produit scalaire C++: " << cpp_result << std::endl;
    std::cout << "Match: " << (std::abs(asm_result - cpp_result) < 0.0001f ? "✓" : "✗") << std::endl;
}

void TestVectorAdd() {
    std::cout << "\n=== Test d'Addition de Vecteurs ===" << std::endl;

    float v1[3] = {1.0f, 2.0f, 3.0f};
    float v2[3] = {4.0f, 5.0f, 6.0f};
    float result[3];

    PhysicsASM::VectorAdd(v1, v2, result);

    PrintVector("Vecteur 1:", v1);
    PrintVector("Vecteur 2:", v2);
    PrintVector("Résultat: ", result);

    bool match = (result[0] == 5.0f && result[1] == 7.0f && result[2] == 9.0f);
    std::cout << "Match: " << (match ? "✓" : "✗") << std::endl;
}

void TestVectorScale() {
    std::cout << "\n=== Test de Multiplication par un Scalaire ===" << std::endl;

    float v[3] = {1.0f, 2.0f, 3.0f};
    float scalar = 2.5f;
    float result[3];

    PhysicsASM::VectorScale(v, scalar, result);

    PrintVector("Vecteur original:", v);
    std::cout << "Scalaire: " << scalar << std::endl;
    PrintVector("Résultat:       ", result);

    bool match = (std::abs(result[0] - 2.5f) < 0.0001f &&
                  std::abs(result[1] - 5.0f) < 0.0001f &&
                  std::abs(result[2] - 7.5f) < 0.0001f);
    std::cout << "Match: " << (match ? "✓" : "✗") << std::endl;
}

void TestPhysicsScenario() {
    std::cout << "\n=== Scénario de Simulation Physique ===" << std::endl;
    std::cout << "On calcule l'interaction gravitationnelle entre deux objets:" << std::endl;

    // Objet 1: position et masse
    float obj1_pos[3] = {0.0f, 0.0f, 0.0f};
    float obj1_mass = 1.0e22f;

    // Objet 2: position et masse
    float obj2_pos[3] = {1000.0f, 2000.0f, 1500.0f};
    float obj2_mass = 5.0e21f;

    std::cout << "\nObjet 1:" << std::endl;
    PrintVector("  Position:", obj1_pos);
    std::cout << "  Masse: " << obj1_mass << " kg" << std::endl;

    std::cout << "\nObjet 2:" << std::endl;
    PrintVector("  Position:", obj2_pos);
    std::cout << "  Masse: " << obj2_mass << " kg" << std::endl;

    // Calcul de la distance au carré (évite le sqrt, c'est plus rapide)
    float distSq = PhysicsASM::DistanceSquared(
        obj1_pos[0], obj1_pos[1], obj1_pos[2],
        obj2_pos[0], obj2_pos[1], obj2_pos[2]
    );

    std::cout << "\nDistance²: " << distSq << " m²" << std::endl;
    std::cout << "Distance:  " << std::sqrt(distSq) << " m" << std::endl;

    // Calcul de la force gravitationnelle (F = G*m1*m2/r²)
    float force = PhysicsASM::GravitationalForce(obj1_mass, obj2_mass, distSq);
    std::cout << "\nForce gravitationnelle: " << force << " N" << std::endl;

    // Calcul du vecteur direction (normalisé pour avoir une direction unitaire)
    float direction[3] = {
        obj2_pos[0] - obj1_pos[0],
        obj2_pos[1] - obj1_pos[1],
        obj2_pos[2] - obj1_pos[2]
    };
    PhysicsASM::Normalize(direction);

    std::cout << "\nDirection de la force (normalisée):" << std::endl;
    PrintVector("  Direction:", direction);

    // Calcul du vecteur force final (direction * magnitude)
    float force_vector[3];
    PhysicsASM::VectorScale(direction, force, force_vector);

    std::cout << "\nVecteur force final:" << std::endl;
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
                result = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
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
    std::cout << "║   Démo des Fonctions Physiques en Assembleur x86-64       ║" << std::endl;
    std::cout << "║   Avec instructions SIMD pour aller ultra vite            ║" << std::endl;
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
        // On lance tous les tests pour vérifier que tout fonctionne
        TestDistanceSquared();
        TestGravitationalForce();
        TestNormalize();
        TestDotProduct();
        TestVectorAdd();
        TestVectorScale();
        TestPhysicsScenario();

        std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║              Tous les tests sont terminés !                ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
        
        std::cout << "\nTip: Run with --benchmark flag to see performance comparisons" << std::endl;
    }

    return 0;
}

