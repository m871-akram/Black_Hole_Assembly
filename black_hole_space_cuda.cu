// black_hole_space_cuda.cu
// CUDA GPU black hole ray tracer — NVIDIA GPU backend.
//
// Merges host C++ (window, display, N-body) with GPU kernel (geodesic
// integration).  Each CUDA thread traces one pixel; the completed frame
// is copied to host memory and uploaded via glTexSubImage2D each frame.
//
// Physics: true 4-stage Runge-Kutta integration of null geodesics in
// Schwarzschild spherical coordinates, matching the CPU RK4 backend.
//
// Prerequisites (Linux / Windows with NVIDIA GPU):
//   CUDA Toolkit 11+, OpenGL 3.3, GLFW3, GLEW, GLM
// Build target: BlackHole_space_cuda  (CMakeLists.txt, CUDA detection block)

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE

// CUDA runtime must be included before any GL headers.
#include <cuda_runtime.h>

#include "common.hpp"
#include "physics_asm.hpp"

#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <cmath>
#include <vector>
#include <chrono>
#include <algorithm>

using namespace std;
using Clock = std::chrono::high_resolution_clock;

// ─── CUDA error checking ─────────────────────────────────────────────────────
#define CUDA_CHECK(call)                                                    \
    do {                                                                    \
        cudaError_t _e = (call);                                            \
        if (_e != cudaSuccess) {                                            \
            fprintf(stderr, "[CUDA] %s:%d  %s\n",                          \
                    __FILE__, __LINE__, cudaGetErrorString(_e));            \
            exit(1);                                                        \
        }                                                                   \
    } while (0)

// ═══════════════════════════════════════════════════════════════════════════════
//  DEVICE CODE  — runs on the GPU
// ═══════════════════════════════════════════════════════════════════════════════

static __device__ __constant__ float d_SagA_rs = 1.269e10f;
static __device__ __constant__ float d_D_LAMBDA = 1e7f;

// Ray in Schwarzschild spherical coordinates
struct __align__(16) DevRay {
    float x, y, z;          // Cartesian position (recomputed after each step)
    float r, theta, phi;    // Spherical position
    float dr, dtheta, dphi; // First derivatives w.r.t. affine parameter λ
    float E, L;             // Conserved energy and angular momentum
};

// Camera data passed to the kernel
struct DevCamera {
    float3 pos, right, up, fwd;
    float  tanHFov, aspect;
};

// Scene object (sphere)
struct DevObject {
    float4 posRadius;   // xyz = centre, w = radius
    float4 color;       // rgba
};

// ─── Device physics helpers ───────────────────────────────────────────────────

__device__ static DevRay d_initRay(float3 pos, float3 dir) {
    DevRay ray;
    ray.x = pos.x;  ray.y = pos.y;  ray.z = pos.z;
    ray.r     = sqrtf(pos.x*pos.x + pos.y*pos.y + pos.z*pos.z);
    ray.theta = acosf(pos.z / ray.r);
    ray.phi   = atan2f(pos.y, pos.x);

    float st = sinf(ray.theta), ct = cosf(ray.theta);
    float sp = sinf(ray.phi),   cp = cosf(ray.phi);

    ray.dr     =  st*cp*dir.x + st*sp*dir.y + ct*dir.z;
    ray.dtheta = (ct*cp*dir.x + ct*sp*dir.y - st*dir.z) / ray.r;
    ray.dphi   = (-sp*dir.x  + cp*dir.y)               / (ray.r * st);

    ray.L = ray.r * ray.r * st * ray.dphi;

    float f     = 1.0f - d_SagA_rs / ray.r;
    float dt_dL = sqrtf((ray.dr * ray.dr) / f
                        + ray.r * ray.r * (ray.dtheta * ray.dtheta
                                           + st * st * ray.dphi * ray.dphi));
    ray.E = f * dt_dL;
    return ray;
}

__device__ static void d_geodesicRHS(const DevRay& ray,
                                      float3& d1, float3& d2) {
    float r      = ray.r,      theta  = ray.theta;
    float dr     = ray.dr,     dtheta = ray.dtheta, dphi = ray.dphi;
    float f      = 1.0f - d_SagA_rs / r;
    float dt_dL  = ray.E / f;
    float st     = sinf(theta), ct = cosf(theta);

    d1 = make_float3(dr, dtheta, dphi);

    d2.x = -(d_SagA_rs / (2.0f * r * r)) * f  * dt_dL * dt_dL
           + (d_SagA_rs / (2.0f * r * r * f)) * dr * dr
           + r * (dtheta * dtheta + st * st * dphi * dphi);
    d2.y = -2.0f * dr * dtheta / r + st * ct * dphi * dphi;
    d2.z = -2.0f * dr * dphi   / r - 2.0f * ct / st * dtheta * dphi;
}

__device__ static DevRay d_applyDelta(const DevRay& base,
                                       const float3& d1, const float3& d2,
                                       float h) {
    DevRay r = base;
    r.r      += h * d1.x;
    r.theta  += h * d1.y;
    r.phi    += h * d1.z;
    r.dr     += h * d2.x;
    r.dtheta += h * d2.y;
    r.dphi   += h * d2.z;
    return r;
}

// True 4-stage Runge-Kutta step (fourth-order accuracy in D_LAMBDA)
__device__ static void d_rk4Step(DevRay& ray, float dL) {
    float3 k1a, k1b, k2a, k2b, k3a, k3b, k4a, k4b;

    d_geodesicRHS(ray, k1a, k1b);
    d_geodesicRHS(d_applyDelta(ray, k1a, k1b, dL * 0.5f), k2a, k2b);
    d_geodesicRHS(d_applyDelta(ray, k2a, k2b, dL * 0.5f), k3a, k3b);
    d_geodesicRHS(d_applyDelta(ray, k3a, k3b, dL),         k4a, k4b);

    const float c = dL / 6.0f;
    ray.r      += c * (k1a.x + 2.0f*k2a.x + 2.0f*k3a.x + k4a.x);
    ray.theta  += c * (k1a.y + 2.0f*k2a.y + 2.0f*k3a.y + k4a.y);
    ray.phi    += c * (k1a.z + 2.0f*k2a.z + 2.0f*k3a.z + k4a.z);
    ray.dr     += c * (k1b.x + 2.0f*k2b.x + 2.0f*k3b.x + k4b.x);
    ray.dtheta += c * (k1b.y + 2.0f*k2b.y + 2.0f*k3b.y + k4b.y);
    ray.dphi   += c * (k1b.z + 2.0f*k2b.z + 2.0f*k3b.z + k4b.z);

    // Reproject to Cartesian for intersection tests
    float st = sinf(ray.theta), ct = cosf(ray.theta);
    float sp = sinf(ray.phi),   cp = cosf(ray.phi);
    ray.x = ray.r * st * cp;
    ray.y = ray.r * st * sp;
    ray.z = ray.r * ct;
}

__device__ static bool d_crossesEquatorial(float3 oldP, float3 newP,
                                             float r1, float r2) {
    bool crossed = (oldP.y * newP.y < 0.0f);
    float r = sqrtf(newP.x * newP.x + newP.z * newP.z);
    return crossed && (r >= r1 && r <= r2);
}

// ─── Main kernel: one thread per pixel ────────────────────────────────────────
__global__ void traceGeodesic(
    unsigned char* __restrict__ pixelBuf,
    int W, int H,
    DevCamera cam,
    const DevObject* __restrict__ objects,
    int numObjects,
    float disk_r1, float disk_r2)
{
    int px = blockIdx.x * blockDim.x + threadIdx.x;
    int py = blockIdx.y * blockDim.y + threadIdx.y;
    if (px >= W || py >= H) return;

    // Generate camera ray direction
    float u = (2.0f*(px + 0.5f)/W  - 1.0f) * cam.aspect * cam.tanHFov;
    float v = (1.0f - 2.0f*(py + 0.5f)/H)  * cam.tanHFov;

    // dir = normalize(u*right - v*up + fwd)
    float3 dir;
    dir.x = u*cam.right.x - v*cam.up.x + cam.fwd.x;
    dir.y = u*cam.right.y - v*cam.up.y + cam.fwd.y;
    dir.z = u*cam.right.z - v*cam.up.z + cam.fwd.z;
    float ilen = rsqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    dir.x *= ilen;  dir.y *= ilen;  dir.z *= ilen;

    DevRay ray = d_initRay(cam.pos, dir);

    bool hitBH = false, hitDisk = false, hitObj = false;
    float4 objColor = make_float4(0,0,0,0);
    float3 objCenter = make_float3(0,0,0);
    float3 prevPos = make_float3(ray.x, ray.y, ray.z);
    const float ESCAPE_R = 1e30f;

    for (int i = 0; i < 60000; ++i) {
        if (ray.r <= d_SagA_rs) { hitBH = true; break; }
        d_rk4Step(ray, d_D_LAMBDA);

        float3 newPos = make_float3(ray.x, ray.y, ray.z);

        if (d_crossesEquatorial(prevPos, newPos, disk_r1, disk_r2)) {
            hitDisk = true; break;
        }

        for (int j = 0; j < numObjects; ++j) {
            float4 pr = objects[j].posRadius;
            float dx = ray.x - pr.x, dy = ray.y - pr.y, dz = ray.z - pr.z;
            float dist = sqrtf(dx*dx + dy*dy + dz*dz);
            if (dist <= pr.w) {
                objColor  = objects[j].color;
                objCenter = make_float3(pr.x, pr.y, pr.z);
                hitObj = true;
                break;
            }
        }
        if (hitObj) break;
        prevPos = newPos;
        if (ray.r > ESCAPE_R) break;
    }

    // Colour mapping — identical to CPU / original GLSL shader
    float cr = 0, cg = 0, cb = 0, ca = 0;
    if (hitDisk) {
        float rv = sqrtf(ray.x*ray.x + ray.y*ray.y + ray.z*ray.z) / disk_r2;
        cr = 1.0f; cg = rv; cb = 0.2f; ca = rv;
    } else if (hitBH) {
        ca = 1.0f;
    } else if (hitObj) {
        float nx = ray.x - objCenter.x, ny = ray.y - objCenter.y,
              nz = ray.z - objCenter.z;
        float nl = rsqrtf(nx*nx + ny*ny + nz*nz);
        nx *= nl;  ny *= nl;  nz *= nl;

        float vx = cam.pos.x - ray.x, vy = cam.pos.y - ray.y,
              vz = cam.pos.z - ray.z;
        float vl = rsqrtf(vx*vx + vy*vy + vz*vz);
        vx *= vl;  vy *= vl;  vz *= vl;

        float ambient   = 0.1f;
        float diff      = fmaxf(nx*vx + ny*vy + nz*vz, 0.0f);
        float intensity = ambient + (1.0f - ambient) * diff;
        cr = objColor.x * intensity;
        cg = objColor.y * intensity;
        cb = objColor.z * intensity;
        ca = objColor.w;
    }

    int idx = 4 * (py * W + px);
    pixelBuf[idx+0] = (unsigned char)(fminf(cr, 1.0f) * 255.0f);
    pixelBuf[idx+1] = (unsigned char)(fminf(cg, 1.0f) * 255.0f);
    pixelBuf[idx+2] = (unsigned char)(fminf(cb, 1.0f) * 255.0f);
    pixelBuf[idx+3] = (unsigned char)(fminf(ca, 1.0f) * 255.0f);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  HOST CODE  — runs on the CPU
// ═══════════════════════════════════════════════════════════════════════════════

double lastPrintTime = 0.0;
int    framesCount   = 0;
bool   Gravity       = false;

struct Camera : public OrbitCamera {
    Camera() : OrbitCamera(vec3(0.0f), 6.34194e10f, 1e10f, 1e12f, 60.0f) {
        elevation  = float(M_PI) / 2.0f;
        orbitSpeed = 0.01f;
        zoomSpeed  = 25e9f;
    }
    void processKey(int key, int /*sc*/, int action, int /*mods*/) {
        if (action == GLFW_PRESS && key == GLFW_KEY_G) {
            Gravity = !Gravity;
            cout << "[CUDA] Gravity " << (Gravity ? "ON" : "OFF") << "\n";
        }
    }
};
Camera camera;

struct BlackHoleCuda {
    vec3   position;
    double mass;
    double r_s;
    BlackHoleCuda(vec3 pos, double m) : position(pos), mass(m) {
        r_s = PhysicsUtils::CalculateSchwarzschildRadius(mass);
    }
};
BlackHoleCuda SagA(vec3(0.0f), 8.54e36);

struct ObjectData {
    vec4  posRadius;
    vec4  color;
    float mass;
    vec3  velocity = vec3(0.0f);
};
vector<ObjectData> objects = {
    { vec4(4e11f, 0.0f, 0.0f, 4e10f), vec4(1,1,0,1), 1.98892e30f },
    { vec4(0.0f, 0.0f, 4e11f, 4e10f), vec4(1,0,0,1), 1.98892e30f },
    { vec4(0.0f, 0.0f, 0.0f, float(SagA.r_s)), vec4(0,0,0,1), float(SagA.mass) },
};

struct CudaEngine {
    GLFWwindow* window = nullptr;
    GLuint quadVAO = 0, quadVBO = 0;
    GLuint texture = 0;
    GLuint shaderProgram = 0;

    int WIDTH = 800, HEIGHT = 600;
    static constexpr int CW = 200, CH = 150;

    // Device buffers
    unsigned char* d_pixelBuf = nullptr;
    DevObject*     d_objects  = nullptr;

    // Host pixel buffer for transfer
    vector<unsigned char> h_pixelBuf;

    CudaEngine() {
        window = WindowManager::CreateWindow(WIDTH, HEIGHT,
                                             "Black Hole – CUDA GPU", 3, 3);
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        WIDTH = fbW;  HEIGHT = fbH;

        h_pixelBuf.assign(CW * CH * 4, 0);

        // Allocate device pixel buffer
        CUDA_CHECK(cudaMalloc(&d_pixelBuf, CW * CH * 4));
        // Allocate device object array (16 max)
        CUDA_CHECK(cudaMalloc(&d_objects, 16 * sizeof(DevObject)));

        // OpenGL quad + texture setup
        shaderProgram = ShaderUtils::CreateProgram(
            R"(#version 330 core
               layout(location=0) in vec2 aPos;
               layout(location=1) in vec2 aUV;
               out vec2 TexCoord;
               void main() { gl_Position = vec4(aPos, 0.0, 1.0); TexCoord = aUV; })",
            R"(#version 330 core
               in vec2 TexCoord;
               out vec4 FragColor;
               uniform sampler2D screenTexture;
               void main() { FragColor = texture(screenTexture, TexCoord); })"
        );

        float verts[] = {
            -1.f,  1.f,  0.f, 1.f,
            -1.f, -1.f,  0.f, 0.f,
             1.f, -1.f,  1.f, 0.f,
            -1.f,  1.f,  0.f, 1.f,
             1.f, -1.f,  1.f, 0.f,
             1.f,  1.f,  1.f, 1.f
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float),
                              (void*)(2*sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, CW, CH, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }

    ~CudaEngine() {
        if (d_pixelBuf) cudaFree(d_pixelBuf);
        if (d_objects)  cudaFree(d_objects);
    }

    void renderGpu(const Camera& cam) {
        // Build camera struct
        vec3 fwd   = normalize(cam.target - cam.position());
        vec3 up    = vec3(0.0f, 1.0f, 0.0f);
        vec3 right = normalize(cross(fwd, up));
        up         = cross(right, fwd);
        vec3 pos   = cam.position();

        DevCamera dcam;
        dcam.pos   = make_float3(pos.x,   pos.y,   pos.z);
        dcam.right = make_float3(right.x, right.y, right.z);
        dcam.up    = make_float3(up.x,    up.y,    up.z);
        dcam.fwd   = make_float3(fwd.x,   fwd.y,   fwd.z);
        dcam.tanHFov = tanf(glm::radians(60.0f * 0.5f));
        dcam.aspect  = float(WIDTH) / float(HEIGHT);

        // Upload object array to device
        int numObj = int(std::min(objects.size(), size_t(16)));
        vector<DevObject> hObjs(numObj);
        for (int i = 0; i < numObj; ++i) {
            hObjs[i].posRadius = make_float4(objects[i].posRadius.x,
                                              objects[i].posRadius.y,
                                              objects[i].posRadius.z,
                                              objects[i].posRadius.w);
            hObjs[i].color     = make_float4(objects[i].color.r,
                                              objects[i].color.g,
                                              objects[i].color.b,
                                              objects[i].color.a);
        }
        CUDA_CHECK(cudaMemcpy(d_objects, hObjs.data(),
                              numObj * sizeof(DevObject), cudaMemcpyHostToDevice));

        float disk_r1 = float(SagA.r_s * 2.2);
        float disk_r2 = float(SagA.r_s * 5.2);

        // Launch kernel — 16×16 thread blocks
        dim3 block(16, 16);
        dim3 grid((CW + 15) / 16, (CH + 15) / 16);
        traceGeodesic<<<grid, block>>>(d_pixelBuf, CW, CH,
                                       dcam, d_objects, numObj,
                                       disk_r1, disk_r2);
        CUDA_CHECK(cudaGetLastError());
        CUDA_CHECK(cudaDeviceSynchronize());

        // Copy result to host, upload to OpenGL texture
        CUDA_CHECK(cudaMemcpy(h_pixelBuf.data(), d_pixelBuf,
                              CW * CH * 4, cudaMemcpyDeviceToHost));
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CW, CH,
                        GL_RGBA, GL_UNSIGNED_BYTE, h_pixelBuf.data());
    }

    void drawQuad() {
        glUseProgram(shaderProgram);
        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(shaderProgram, "screenTexture"), 0);
        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEnable(GL_DEPTH_TEST);
    }
};

CudaEngine* g_engine = nullptr;

int main() {
    // Print CUDA device info
    int devCount = 0;
    CUDA_CHECK(cudaGetDeviceCount(&devCount));
    if (devCount == 0) {
        fprintf(stderr, "[CUDA] No CUDA-capable device found.\n");
        return 1;
    }
    cudaDeviceProp prop{};
    CUDA_CHECK(cudaGetDeviceProperties(&prop, 0));
    printf("[CUDA] Device: %s  (SM %d.%d)\n",
           prop.name, prop.major, prop.minor);

    CudaEngine eng;
    g_engine = &eng;

    OrbitCamera::RegisterCallbacks(eng.window, &camera);

    glfwSetKeyCallback(eng.window,
        [](GLFWwindow*, int key, int sc, int action, int mods) {
            camera.processKey(key, sc, action, mods);
        });

    glfwSetFramebufferSizeCallback(eng.window,
        [](GLFWwindow*, int w, int h) {
            g_engine->WIDTH  = w;
            g_engine->HEIGHT = h;
            glViewport(0, 0, w, h);
        });

    auto   t0        = Clock::now();
    lastPrintTime    = chrono::duration<double>(t0.time_since_epoch()).count();
    double lastTime  = glfwGetTime();

    while (!glfwWindowShouldClose(eng.window)) {
        double now = glfwGetTime();
        double dt  = now - lastTime;
        lastTime   = now;

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // N-body gravity (host-side, same as other backends)
        for (auto& obj : objects) {
            for (auto& obj2 : objects) {
                if (&obj == &obj2) continue;
                float dx   = obj2.posRadius.x - obj.posRadius.x;
                float dy   = obj2.posRadius.y - obj.posRadius.y;
                float dz   = obj2.posRadius.z - obj.posRadius.z;
                float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                if (dist > 0 && Gravity) {
                    vec3   direction = normalize(vec3(dx, dy, dz));
                    double Gforce    = (GRAVITATIONAL_CONSTANT * obj.mass * obj2.mass)
                                       / (double(dist) * double(dist));
                    double acc1      = Gforce / obj.mass;
                    vec3   acc       = direction * float(acc1);
                    obj.velocity    += acc * float(dt);
                    obj.posRadius.x += obj.velocity.x * float(dt);
                    obj.posRadius.y += obj.velocity.y * float(dt);
                    obj.posRadius.z += obj.velocity.z * float(dt);
                }
            }
        }

        glViewport(0, 0, eng.WIDTH, eng.HEIGHT);
        eng.renderGpu(camera);
        eng.drawQuad();

        glfwSwapBuffers(eng.window);
        glfwPollEvents();

        ++framesCount;
        double nowD = chrono::duration<double>(Clock::now().time_since_epoch()).count();
        if (nowD - lastPrintTime >= 1.0) {
            printf("[CUDA-RK4] %d fps  |  %s\n", framesCount, prop.name);
            framesCount   = 0;
            lastPrintTime = nowD;
        }
    }

    glfwDestroyWindow(eng.window);
    glfwTerminate();
    return 0;
}
