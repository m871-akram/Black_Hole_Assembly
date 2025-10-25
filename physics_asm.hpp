#ifndef PHYSICS_ASM_HPP
#define PHYSICS_ASM_HPP

#include <cstddef>

// Interface C++ pour les fonctions assembleur ultra-optimisées
// Bon bah là c'est simple, on a écrit tout en ASM x86-64 avec SSE
// pcq les calculs physiques ça doit aller VITE sinon ça lag
// genre flemme d'attendre que le CPU fasse les calculs un par un

extern "C" {
    // Calcule la distance au carré entre deux points 3D
    // On évite le sqrt() pcq c'est ultra lent, on garde r² c'est suffisant
    // pour comparer les distances (genre qui est plus proche)
    float vector_distance_squared(float x1, float y1, float z1,
                                   float x2, float y2, float z2);

    // Force gravitationnelle selon Newton : F = G * m1 * m2 / r²
    // Bon là faut que les masses soient en kg et distance en m sinon ça donne nawak
    float gravitational_force(float mass1, float mass2, float distance_squared);

    // Normalise un vecteur 3D (met sa longueur à 1)
    // Attention ça modifie le vecteur direct, pas de copie donc
    void normalize_vector3(float* vector);

    // Produit scalaire de deux vecteurs 3D
    // résultat = v1 · v2 = x1*x2 + y1*y2 + z1*z2
    // pratique pour calculer les angles et tout
    float dot_product3(const float* v1, const float* v2);

    // Addition de deux vecteurs 3D : result = v1 + v2
    // basique mais optimisé en ASM donc ça va super vite
    void vector_add3(const float* v1, const float* v2, float* result);

    // Multiplie un vecteur par un scalaire : result = v * scalar
    // genre pour appliquer une force ou changer la vitesse
    void vector_scale3(const float* v, float scalar, float* result);
}

// Namespace C++ pour utiliser les fonctions plus facilement
// Genre au lieu d'appeler vector_distance_squared() t'appelles PhysicsASM::DistanceSquared()
// c'est juste des wrappers mais ça fait propre
namespace PhysicsASM {
    // Calcule la distance au carré (wrapper pour faire classe)
    inline float DistanceSquared(float x1, float y1, float z1,
                                  float x2, float y2, float z2) {
        return vector_distance_squared(x1, y1, z1, x2, y2, z2);
    }

    // Force gravitationnelle (wrapper aussi)
    inline float GravitationalForce(float mass1, float mass2, float distSq) {
        return gravitational_force(mass1, mass2, distSq);
    }

    // Normalise un vecteur (devine quoi, c'est un wrapper mdr)
    inline void Normalize(float* vec) {
        normalize_vector3(vec);
    }

    // Produit scalaire entre deux vecteurs (t'as capté c'est un wrapper)
    inline float DotProduct(const float* v1, const float* v2) {
        return dot_product3(v1, v2);
    }

    // Additionne deux vecteurs (ouais encore un wrapper lol)
    inline void VectorAdd(const float* v1, const float* v2, float* result) {
        vector_add3(v1, v2, result);
    }

    // Multiplie un vecteur par un scalaire (dernier wrapper promis)
    inline void VectorScale(const float* v, float scalar, float* result) {
        vector_scale3(v, scalar, result);
    }
}

#endif // PHYSICS_ASM_HPP

