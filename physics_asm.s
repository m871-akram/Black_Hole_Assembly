/* 
 * Ce fichier implémente des opérations mathématiques optimisées pour les
 * simulations de physique gravitationnelle utilisant les instructions SIMD (SSE).
 *
 * Cible: x86-64 (Linux/macOS)
 * Syntaxe: AT&T (GNU Assembler)
 * Convention d'appel: System V AMD64 ABI
 * ============================================================================
 */


/* ============================================================================
 * vector_distance_squared
 * Calcule la distance au carré entre deux points 3D (évite le coût élevé de sqrt)
 *
 * Paramètres (System V AMD64 ABI):
 *   %xmm0 = x1 (flottant)
 *   %xmm1 = y1 (flottant)
 *   %xmm2 = z1 (flottant)
 *   %xmm3 = x2 (flottant)
 *   %xmm4 = y2 (flottant)
 *   %xmm5 = z2 (flottant)
 *
 * Retour:
 *   %xmm0 = distance au carré (flottant)
 * ============================================================================
 */
    .globl vector_distance_squared
    .type vector_distance_squared, @function
vector_distance_squared:
    /* Calcul de dx = x2 - x1 */
    subss   %xmm0, %xmm3        /* xmm3 = x2 - x1 = dx */

    /* Calcul de dy = y2 - y1 */
    subss   %xmm1, %xmm4        /* xmm4 = y2 - y1 = dy */

    /* Calcul de dz = z2 - z1 */
    subss   %xmm2, %xmm5        /* xmm5 = z2 - z1 = dz */

    /* Calcul de dx^2 */
    mulss   %xmm3, %xmm3        /* xmm3 = dx * dx */

    /* Calcul de dy^2 */
    mulss   %xmm4, %xmm4        /* xmm4 = dy * dy */

    /* Calcul de dz^2 */
    mulss   %xmm5, %xmm5        /* xmm5 = dz * dz */

    /* Somme: dx^2 + dy^2 */
    addss   %xmm4, %xmm3        /* xmm3 = dx^2 + dy^2 */

    /* Somme: dx^2 + dy^2 + dz^2 */
    addss   %xmm5, %xmm3        /* xmm3 = dx^2 + dy^2 + dz^2 */

    /* Déplacement du résultat vers xmm0 (registre de retour) */
    movss   %xmm3, %xmm0

    ret
    .size vector_distance_squared, .-vector_distance_squared

/* ============================================================================
 * gravitational_force
 * Calcule la magnitude de la force gravitationnelle: F = G * m1 * m2 / r^2
 *
 * Paramètres (System V AMD64 ABI):
 *   %xmm0 = masse1 (flottant)
 *   %xmm1 = masse2 (flottant)
 *   %xmm2 = distance_au_carré (flottant)
 *
 * Retour:
 *   %xmm0 = magnitude de la force (flottant)
 * ============================================================================
 */
    .globl gravitational_force
    .type gravitational_force, @function
gravitational_force:
    /* Chargement de la constante gravitationnelle G = 6.67430e-11 */
    movss   grav_const(%rip), %xmm3     /* xmm3 = G */

    /* Réordonnancement des opérations pour éviter le débordement: F = (G * m1 / r^2) * m2 */

    /* Calcul de G * m1 */
    mulss   %xmm3, %xmm0                /* xmm0 = G * m1 */

    /* Calcul de (G * m1) / r^2 */
    divss   %xmm2, %xmm0                /* xmm0 = (G * m1) / r^2 */

    /* Calcul de F = ((G * m1) / r^2) * m2 */
    mulss   %xmm1, %xmm0                /* xmm0 = F */

    ret
    .size gravitational_force, .-gravitational_force

/* ============================================================================
 * normalize_vector3
 * Normalise un vecteur 3D (rend sa longueur égale à 1)
 *
 * Paramètres (System V AMD64 ABI):
 *   %rdi = pointeur vers le vecteur entrée/sortie [x, y, z] (tableau de flottants)
 *
 * Retour:
 *   void (modifie le vecteur sur place)
 * ============================================================================
 */
    .globl normalize_vector3
    .type normalize_vector3, @function
normalize_vector3:
    /* Chargement des composantes du vecteur */
    movss   (%rdi), %xmm0           /* xmm0 = x */
    movss   4(%rdi), %xmm1          /* xmm1 = y */
    movss   8(%rdi), %xmm2          /* xmm2 = z */

    /* Calcul de x^2 */
    movss   %xmm0, %xmm3
    mulss   %xmm3, %xmm3            /* xmm3 = x^2 */

    /* Calcul de y^2 */
    movss   %xmm1, %xmm4
    mulss   %xmm4, %xmm4            /* xmm4 = y^2 */

    /* Calcul de z^2 */
    movss   %xmm2, %xmm5
    mulss   %xmm5, %xmm5            /* xmm5 = z^2 */

    /* Calcul de magnitude^2 = x^2 + y^2 + z^2 */
    addss   %xmm4, %xmm3
    addss   %xmm5, %xmm3            /* xmm3 = mag^2 */

    /* Calcul de magnitude = sqrt(mag^2) */
    sqrtss  %xmm3, %xmm3            /* xmm3 = magnitude */

    /* Vérification si la magnitude est nulle (éviter la division par zéro) */
    xorps   %xmm6, %xmm6            /* xmm6 = 0.0 */
    ucomiss %xmm6, %xmm3
    je      .Lskip_normalize        /* si mag == 0, ignorer la normalisation */

    /* Normalisation: x / magnitude */
    divss   %xmm3, %xmm0
    movss   %xmm0, (%rdi)

    /* Normalisation: y / magnitude */
    divss   %xmm3, %xmm1
    movss   %xmm1, 4(%rdi)

    /* Normalisation: z / magnitude */
    divss   %xmm3, %xmm2
    movss   %xmm2, 8(%rdi)

.Lskip_normalize:
    ret
    .size normalize_vector3, .-normalize_vector3

/* ============================================================================
 * dot_product3
 * Calcule le produit scalaire de deux vecteurs 3D: v1 · v2 = x1*x2 + y1*y2 + z1*z2
 *
 * Paramètres (System V AMD64 ABI):
 *   %rdi = pointeur vers le vecteur1 [x1, y1, z1] (tableau de flottants)
 *   %rsi = pointeur vers le vecteur2 [x2, y2, z2] (tableau de flottants)
 *
 * Retour:
 *   %xmm0 = produit scalaire (flottant)
 * ============================================================================
 */
    .globl dot_product3
    .type dot_product3, @function
dot_product3:
    /* Chargement du premier vecteur */
    movss   (%rdi), %xmm0           /* xmm0 = x1 */
    movss   4(%rdi), %xmm1          /* xmm1 = y1 */
    movss   8(%rdi), %xmm2          /* xmm2 = z1 */

    /* Chargement du second vecteur */
    movss   (%rsi), %xmm3           /* xmm3 = x2 */
    movss   4(%rsi), %xmm4          /* xmm4 = y2 */
    movss   8(%rsi), %xmm5          /* xmm5 = z2 */

    /* Multiplication des composantes */
    mulss   %xmm3, %xmm0            /* xmm0 = x1 * x2 */
    mulss   %xmm4, %xmm1            /* xmm1 = y1 * y2 */
    mulss   %xmm5, %xmm2            /* xmm2 = z1 * z2 */

    /* Somme des produits */
    addss   %xmm1, %xmm0            /* xmm0 = x1*x2 + y1*y2 */
    addss   %xmm2, %xmm0            /* xmm0 = x1*x2 + y1*y2 + z1*z2 */

    ret
    .size dot_product3, .-dot_product3

/* ============================================================================
 * vector_add3
 * Additionne deux vecteurs 3D: résultat = v1 + v2
 *
 * Paramètres (System V AMD64 ABI):
 *   %rdi = pointeur vers le vecteur1 [x1, y1, z1] (tableau de flottants)
 *   %rsi = pointeur vers le vecteur2 [x2, y2, z2] (tableau de flottants)
 *   %rdx = pointeur vers le vecteur résultat (tableau de flottants)
 *
 * Retour:
 *   void (stocke le résultat dans rdx)
 * ============================================================================
 */
    .globl vector_add3
    .type vector_add3, @function
vector_add3:
    /* Chargement et addition des composantes x */
    movss   (%rdi), %xmm0           /* xmm0 = x1 */
    addss   (%rsi), %xmm0           /* xmm0 = x1 + x2 */
    movss   %xmm0, (%rdx)           /* stockage du résultat */

    /* Chargement et addition des composantes y */
    movss   4(%rdi), %xmm1          /* xmm1 = y1 */
    addss   4(%rsi), %xmm1          /* xmm1 = y1 + y2 */
    movss   %xmm1, 4(%rdx)          /* stockage du résultat */

    /* Chargement et addition des composantes z */
    movss   8(%rdi), %xmm2          /* xmm2 = z1 */
    addss   8(%rsi), %xmm2          /* xmm2 = z1 + z2 */
    movss   %xmm2, 8(%rdx)          /* stockage du résultat */

    ret
    .size vector_add3, .-vector_add3

/* ============================================================================
 * vector_scale3
 * Met à l'échelle un vecteur 3D par un scalaire: résultat = v * scalaire
 *
 * Paramètres (System V AMD64 ABI):
 *   %rdi = pointeur vers le vecteur entrée [x, y, z] (tableau de flottants)
 *   %xmm0 = valeur scalaire (flottant)
 *   %rsi = pointeur vers le vecteur résultat (tableau de flottants)
 *
 * Retour:
 *   void (stocke le résultat dans rsi)
 * ============================================================================
 */
    .globl vector_scale3
    .type vector_scale3, @function
vector_scale3:
    /* Mise à l'échelle de la composante x */
    movss   (%rdi), %xmm1           /* xmm1 = x */
    mulss   %xmm0, %xmm1            /* xmm1 = x * scalaire */
    movss   %xmm1, (%rsi)           /* stockage du résultat */

    /* Mise à l'échelle de la composante y */
    movss   4(%rdi), %xmm2          /* xmm2 = y */
    mulss   %xmm0, %xmm2            /* xmm2 = y * scalaire */
    movss   %xmm2, 4(%rsi)          /* stockage du résultat */

    /* Mise à l'échelle de la composante z */
    movss   8(%rdi), %xmm3          /* xmm3 = z */
    mulss   %xmm0, %xmm3            /* xmm3 = z * scalaire */
    movss   %xmm3, 8(%rsi)          /* stockage du résultat */

    ret
    .size vector_scale3, .-vector_scale3

/* ============================================================================
 * Section de données - constantes
 * ============================================================================
 */
    .data
    .align 16
grav_const:
    .float 6.67430e-11              /* Constante gravitationnelle G */

    .section .note.GNU-stack,"",@progbits
