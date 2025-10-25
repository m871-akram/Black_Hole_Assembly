; ============================================================================
; physics_asm.s - High-performance physics calculations in x86-64 Assembly
; ============================================================================
; This file implements optimized mathematical operations for gravitational
; physics simulations using SIMD instructions (SSE/AVX).
;
; Author: Mohammed Akram Lrhorfi
; Target: x86-64 (macOS/Linux)
; Calling Convention: System V AMD64 ABI (used on macOS and Linux)
; ============================================================================

section .text
    global _vector_distance_squared    ; Calculate squared distance between two 3D points
    global _gravitational_force        ; Calculate gravitational force between two masses
    global _normalize_vector3          ; Normalize a 3D vector
    global _dot_product3               ; Dot product of two 3D vectors
    global _vector_add3                ; Add two 3D vectors
    global _vector_scale3              ; Scale a 3D vector by a scalar

; ============================================================================
; _vector_distance_squared
; Calculates the squared distance between two 3D points (avoids expensive sqrt)
;
; Parameters (System V AMD64 ABI):
;   xmm0 = x1 (float)
;   xmm1 = y1 (float)
;   xmm2 = z1 (float)
;   xmm3 = x2 (float)
;   xmm4 = y2 (float)
;   xmm5 = z2 (float)
;
; Returns:
;   xmm0 = distance squared (float)
; ============================================================================
_vector_distance_squared:
    ; Calculate dx = x2 - x1
    subss   xmm3, xmm0              ; xmm3 = dx

    ; Calculate dy = y2 - y1
    subss   xmm4, xmm1              ; xmm4 = dy

    ; Calculate dz = z2 - z1
    subss   xmm5, xmm2              ; xmm5 = dz

    ; Calculate dx^2
    mulss   xmm3, xmm3              ; xmm3 = dx^2

    ; Calculate dy^2
    mulss   xmm4, xmm4              ; xmm4 = dy^2

    ; Calculate dz^2
    mulss   xmm5, xmm5              ; xmm5 = dz^2

    ; Sum: dx^2 + dy^2
    addss   xmm3, xmm4              ; xmm3 = dx^2 + dy^2

    ; Sum: dx^2 + dy^2 + dz^2
    addss   xmm3, xmm5              ; xmm3 = dx^2 + dy^2 + dz^2

    ; Move result to xmm0 (return register)
    movss   xmm0, xmm3

    ret

; ============================================================================
; _gravitational_force
; Calculates gravitational force magnitude: F = G * m1 * m2 / r^2
;
; Parameters (System V AMD64 ABI):
;   xmm0 = mass1 (float)
;   xmm1 = mass2 (float)
;   xmm2 = distance_squared (float)
;
; Returns:
;   xmm0 = force magnitude (float)
; ============================================================================
_gravitational_force:
    ; Load gravitational constant G = 6.67430e-11
    ; We'll use a simplified version for the simulation
    movss   xmm3, [rel grav_const]  ; xmm3 = G

    ; Calculate m1 * m2
    mulss   xmm0, xmm1              ; xmm0 = m1 * m2

    ; Calculate G * m1 * m2
    mulss   xmm0, xmm3              ; xmm0 = G * m1 * m2

    ; Calculate F = (G * m1 * m2) / r^2
    divss   xmm0, xmm2              ; xmm0 = F

    ret

; ============================================================================
; _normalize_vector3
; Normalizes a 3D vector (makes its length = 1)
;
; Parameters (System V AMD64 ABI):
;   rdi = pointer to input/output vector [x, y, z] (float array)
;
; Returns:
;   void (modifies vector in place)
; ============================================================================
_normalize_vector3:
    ; Load vector components
    movss   xmm0, [rdi]             ; xmm0 = x
    movss   xmm1, [rdi + 4]         ; xmm1 = y
    movss   xmm2, [rdi + 8]         ; xmm2 = z

    ; Calculate x^2
    movss   xmm3, xmm0
    mulss   xmm3, xmm3              ; xmm3 = x^2

    ; Calculate y^2
    movss   xmm4, xmm1
    mulss   xmm4, xmm4              ; xmm4 = y^2

    ; Calculate z^2
    movss   xmm5, xmm2
    mulss   xmm5, xmm5              ; xmm5 = z^2

    ; Calculate magnitude^2 = x^2 + y^2 + z^2
    addss   xmm3, xmm4
    addss   xmm3, xmm5              ; xmm3 = mag^2

    ; Calculate magnitude = sqrt(mag^2)
    sqrtss  xmm3, xmm3              ; xmm3 = magnitude

    ; Check if magnitude is zero (avoid division by zero)
    xorps   xmm6, xmm6              ; xmm6 = 0.0
    ucomiss xmm3, xmm6
    je      .skip_normalize         ; if mag == 0, skip normalization

    ; Normalize: x / magnitude
    divss   xmm0, xmm3
    movss   [rdi], xmm0

    ; Normalize: y / magnitude
    divss   xmm1, xmm3
    movss   [rdi + 4], xmm1

    ; Normalize: z / magnitude
    divss   xmm2, xmm3
    movss   [rdi + 8], xmm2

.skip_normalize:
    ret

; ============================================================================
; _dot_product3
; Calculates dot product of two 3D vectors: v1 · v2 = x1*x2 + y1*y2 + z1*z2
;
; Parameters (System V AMD64 ABI):
;   rdi = pointer to vector1 [x1, y1, z1] (float array)
;   rsi = pointer to vector2 [x2, y2, z2] (float array)
;
; Returns:
;   xmm0 = dot product (float)
; ============================================================================
_dot_product3:
    ; Load first vector
    movss   xmm0, [rdi]             ; xmm0 = x1
    movss   xmm1, [rdi + 4]         ; xmm1 = y1
    movss   xmm2, [rdi + 8]         ; xmm2 = z1

    ; Load second vector
    movss   xmm3, [rsi]             ; xmm3 = x2
    movss   xmm4, [rsi + 4]         ; xmm4 = y2
    movss   xmm5, [rsi + 8]         ; xmm5 = z2

    ; Multiply components
    mulss   xmm0, xmm3              ; xmm0 = x1 * x2
    mulss   xmm1, xmm4              ; xmm1 = y1 * y2
    mulss   xmm2, xmm5              ; xmm2 = z1 * z2

    ; Sum the products
    addss   xmm0, xmm1              ; xmm0 = x1*x2 + y1*y2
    addss   xmm0, xmm2              ; xmm0 = x1*x2 + y1*y2 + z1*z2

    ret

; ============================================================================
; _vector_add3
; Adds two 3D vectors: result = v1 + v2
;
; Parameters (System V AMD64 ABI):
;   rdi = pointer to vector1 [x1, y1, z1] (float array)
;   rsi = pointer to vector2 [x2, y2, z2] (float array)
;   rdx = pointer to result vector (float array)
;
; Returns:
;   void (stores result in rdx)
; ============================================================================
_vector_add3:
    ; Load and add x components
    movss   xmm0, [rdi]             ; xmm0 = x1
    addss   xmm0, [rsi]             ; xmm0 = x1 + x2
    movss   [rdx], xmm0             ; store result

    ; Load and add y components
    movss   xmm1, [rdi + 4]         ; xmm1 = y1
    addss   xmm1, [rsi + 4]         ; xmm1 = y1 + y2
    movss   [rdx + 4], xmm1         ; store result

    ; Load and add z components
    movss   xmm2, [rdi + 8]         ; xmm2 = z1
    addss   xmm2, [rsi + 8]         ; xmm2 = z1 + z2
    movss   [rdx + 8], xmm2         ; store result

    ret

; ============================================================================
; _vector_scale3
; Scales a 3D vector by a scalar: result = v * scalar
;
; Parameters (System V AMD64 ABI):
;   rdi = pointer to input vector [x, y, z] (float array)
;   xmm0 = scalar value (float)
;   rsi = pointer to result vector (float array)
;
; Returns:
;   void (stores result in rsi)
; ============================================================================
_vector_scale3:
    ; Scale x component
    movss   xmm1, [rdi]             ; xmm1 = x
    mulss   xmm1, xmm0              ; xmm1 = x * scalar
    movss   [rsi], xmm1             ; store result

    ; Scale y component
    movss   xmm2, [rdi + 4]         ; xmm2 = y
    mulss   xmm2, xmm0              ; xmm2 = y * scalar
    movss   [rsi + 4], xmm2         ; store result

    ; Scale z component
    movss   xmm3, [rdi + 8]         ; xmm3 = z
    mulss   xmm3, xmm0              ; xmm3 = z * scalar
    movss   [rsi + 8], xmm3         ; store result

    ret

; ============================================================================
; Data section - constants
; ============================================================================
section .data
    align 16
    grav_const: dd 6.67430e-11      ; Gravitational constant G

section .note.GNU-stack noalloc noexec nowrite progbits

