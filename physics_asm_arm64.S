// physics_asm_arm64.S
// Native ARM64/AArch64 assembly for the six physics functions.
//
// Target:   ARM64 — Apple Silicon (Mach-O) and Linux AArch64 (ELF)
// ABI:      AAPCS64
//             Scalar float args : s0–s7 (independent of integer bank)
//             Integer/ptr args  : x0–x7
//             Return value      : s0 (float), x0 (pointer/int)
// Assembler: Clang or GCC (preprocessed via .S extension)
//
// Symbol naming:
//   macOS Mach-O  — C symbols require a leading underscore (_foo)
//   Linux ELF     — C symbols have no prefix (foo)
//
// IMPORTANT: On macOS, the ';' character is treated as a COMMENT by Apple's
// assembler (as), so every directive and label is on its own line.
// The #ifdef guards use separate lines for .globl and the label definition.
//
// Functions:
//   float  vector_distance_squared(x1,y1,z1, x2,y2,z2)
//   float  gravitational_force(mass1, mass2, distance_squared)
//   void   normalize_vector3(float* v)        [in-place]
//   float  dot_product3(const float* v1, const float* v2)
//   void   vector_add3(const float* v1, const float* v2, float* result)
//   void   vector_scale3(const float* v, float scalar, float* result)

    .text

// ─────────────────────────────────────────────────────────────────────────────
// vector_distance_squared(x1, y1, z1, x2, y2, z2) -> float
//
// Args:  s0=x1, s1=y1, s2=z1, s3=x2, s4=y2, s5=z2
// ─────────────────────────────────────────────────────────────────────────────
#ifdef __APPLE__
    .globl _vector_distance_squared
_vector_distance_squared:
#else
    .globl vector_distance_squared
    .type  vector_distance_squared, %function
vector_distance_squared:
#endif
    fsub    s3, s3, s0          // dx = x2 - x1
    fsub    s4, s4, s1          // dy = y2 - y1
    fsub    s5, s5, s2          // dz = z2 - z1
    fmul    s3, s3, s3          // dx²
    fmul    s4, s4, s4          // dy²
    fmul    s5, s5, s5          // dz²
    fadd    s3, s3, s4          // dx² + dy²
    fadd    s0, s3, s5          // result: dx² + dy² + dz²
    ret
#ifndef __APPLE__
    .size   vector_distance_squared, .-vector_distance_squared
#endif


// ─────────────────────────────────────────────────────────────────────────────
// gravitational_force(mass1, mass2, distance_squared) -> float
//
// F = G * mass1 * mass2 / distance_squared
// Args:  s0=mass1, s1=mass2, s2=distance_squared
// Uses:  s3 (G constant), x8 (scratch pointer)
// ─────────────────────────────────────────────────────────────────────────────
#ifdef __APPLE__
    .globl _gravitational_force
_gravitational_force:
#else
    .globl gravitational_force
    .type  gravitational_force, %function
gravitational_force:
#endif
#ifdef __APPLE__
    adrp    x8, _grav_const@PAGE
    ldr     s3, [x8, _grav_const@PAGEOFF]
#else
    adrp    x8, grav_const
    ldr     s3, [x8, :lo12:grav_const]
#endif
    fmul    s0, s0, s3          // G * mass1
    fdiv    s0, s0, s2          // G * mass1 / distance_squared
    fmul    s0, s0, s1          // result: G * mass1 / r² * mass2
    ret
#ifndef __APPLE__
    .size   gravitational_force, .-gravitational_force
#endif


// ─────────────────────────────────────────────────────────────────────────────
// normalize_vector3(float* v)  [in-place, void]
//
// Normalises v[3]. No-op if |v|² == 0.
// Args:  x0 = pointer to float[3]
// ─────────────────────────────────────────────────────────────────────────────
#ifdef __APPLE__
    .globl _normalize_vector3
_normalize_vector3:
#else
    .globl normalize_vector3
    .type  normalize_vector3, %function
normalize_vector3:
#endif
    ldr     s0, [x0]            // x
    ldr     s1, [x0, #4]        // y
    ldr     s2, [x0, #8]        // z
    fmul    s3, s0, s0          // x²
    fmul    s4, s1, s1          // y²
    fmul    s5, s2, s2          // z²
    fadd    s3, s3, s4          // x² + y²
    fadd    s3, s3, s5          // magnitude² = x² + y² + z²

    // Skip if zero-length vector
    fcmp    s3, #0.0
    beq     L_skip_normalize

    fsqrt   s3, s3              // magnitude
    fdiv    s0, s0, s3
    fdiv    s1, s1, s3
    fdiv    s2, s2, s3
    str     s0, [x0]
    str     s1, [x0, #4]
    str     s2, [x0, #8]

L_skip_normalize:
    ret
#ifndef __APPLE__
    .size   normalize_vector3, .-normalize_vector3
#endif


// ─────────────────────────────────────────────────────────────────────────────
// dot_product3(const float* v1, const float* v2) -> float
//
// Returns v1·v2 = x1*x2 + y1*y2 + z1*z2
// Args:  x0=v1, x1=v2
// ─────────────────────────────────────────────────────────────────────────────
#ifdef __APPLE__
    .globl _dot_product3
_dot_product3:
#else
    .globl dot_product3
    .type  dot_product3, %function
dot_product3:
#endif
    ldr     s0, [x0]
    ldr     s1, [x0, #4]
    ldr     s2, [x0, #8]
    ldr     s3, [x1]
    ldr     s4, [x1, #4]
    ldr     s5, [x1, #8]
    fmul    s0, s0, s3          // x1*x2
    fmul    s1, s1, s4          // y1*y2
    fmul    s2, s2, s5          // z1*z2
    fadd    s0, s0, s1
    fadd    s0, s0, s2          // result: dot product
    ret
#ifndef __APPLE__
    .size   dot_product3, .-dot_product3
#endif


// ─────────────────────────────────────────────────────────────────────────────
// vector_add3(const float* v1, const float* v2, float* result)  [void]
//
// result[i] = v1[i] + v2[i]
// Args:  x0=v1, x1=v2, x2=result
// ─────────────────────────────────────────────────────────────────────────────
#ifdef __APPLE__
    .globl _vector_add3
_vector_add3:
#else
    .globl vector_add3
    .type  vector_add3, %function
vector_add3:
#endif
    ldr     s0, [x0]
    ldr     s1, [x1]
    fadd    s2, s0, s1
    str     s2, [x2]

    ldr     s0, [x0, #4]
    ldr     s1, [x1, #4]
    fadd    s2, s0, s1
    str     s2, [x2, #4]

    ldr     s0, [x0, #8]
    ldr     s1, [x1, #8]
    fadd    s2, s0, s1
    str     s2, [x2, #8]
    ret
#ifndef __APPLE__
    .size   vector_add3, .-vector_add3
#endif


// ─────────────────────────────────────────────────────────────────────────────
// vector_scale3(const float* v, float scalar, float* result)  [void]
//
// result[i] = v[i] * scalar
//
// AAPCS64 mixed argument banks (independent int/float register allocation):
//   x0 = v (pointer)   s0 = scalar (float)   x1 = result (pointer)
// ─────────────────────────────────────────────────────────────────────────────
#ifdef __APPLE__
    .globl _vector_scale3
_vector_scale3:
#else
    .globl vector_scale3
    .type  vector_scale3, %function
vector_scale3:
#endif
    ldr     s1, [x0]
    fmul    s2, s1, s0
    str     s2, [x1]

    ldr     s1, [x0, #4]
    fmul    s2, s1, s0
    str     s2, [x1, #4]

    ldr     s1, [x0, #8]
    fmul    s2, s1, s0
    str     s2, [x1, #8]
    ret
#ifndef __APPLE__
    .size   vector_scale3, .-vector_scale3
#endif


// ─── Data section: compile-time constants ────────────────────────────────────
#ifdef __APPLE__
    .section __DATA,__data
    .align   4
_grav_const:
#else
    .data
    .align   4
grav_const:
#endif
    .single 6.67430e-11         // Gravitational constant G (m³ kg⁻¹ s⁻²)

// Linux ELF: mark stack as non-executable (ABI / security requirement)
#ifndef __APPLE__
    .section .note.GNU-stack,"",@progbits
#endif
