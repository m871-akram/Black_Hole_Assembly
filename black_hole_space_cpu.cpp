// black_hole_space_cpu.cpp
// CPU-multithreaded Schwarzschild black hole ray tracer.
//
// Uses true 4-stage Runge-Kutta (RK4) to integrate null geodesics in spherical
// coordinates.  Pixel rows are distributed across std::thread workers; the
// completed RGBA8 frame is uploaded to an OpenGL 3.3 texture each frame.
//
// Architecture: hardware-agnostic C++17.  The physics assembly backend is
// selected at link time by CMake (physics_asm_arm64.s on ARM64, physics_asm.s
// on x86-64) — no #ifdef guards needed here.
//
// Build target: BlackHole_space_cpu

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE
#include "common.hpp"
#include "physics_asm.hpp"
#define _USE_MATH_DEFINES
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <thread>
#include <vector>
#include <cmath>
#include <algorithm>
#include <chrono>

using namespace std;
using Clock = std::chrono::high_resolution_clock;

// ─── Scene globals ────────────────────────────────────────────────────────────
double lastPrintTime = 0.0;
int    framesCount   = 0;
bool   Gravity       = false;

// ─── Camera ──────────────────────────────────────────────────────────────────
struct Camera : public OrbitCamera {
    Camera() : OrbitCamera(vec3(0.0f), 6.34194e10f, 1e10f, 1e12f, 60.0f) {
        elevation  = float(M_PI) / 2.0f;
        orbitSpeed = 0.01f;
        zoomSpeed  = 25e9f;
    }
    void processKey(int key, int /*sc*/, int action, int /*mods*/) {
        if (action == GLFW_PRESS && key == GLFW_KEY_G) {
            Gravity = !Gravity;
            cout << "[CPU] Gravity " << (Gravity ? "ON" : "OFF") << "\n";
        }
    }
};
Camera camera;

// ─── Scene objects ────────────────────────────────────────────────────────────
struct BlackHole {
    vec3   position;
    double mass;
    double r_s;
    BlackHole(vec3 pos, double m) : position(pos), mass(m) {
        r_s = PhysicsUtils::CalculateSchwarzschildRadius(mass);
    }
};
BlackHole SagA(vec3(0.0f), 8.54e36);

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

// ─── Physics constants ────────────────────────────────────────────────────────
static constexpr float  SagA_rs  = 1.269e10f;
static constexpr float  D_LAMBDA = 1e7f;
static constexpr double ESCAPE_R = 1e30;

// ─── Ray struct ───────────────────────────────────────────────────────────────
struct Ray {
    float x, y, z;          // Cartesian position (recomputed each step)
    float r, theta, phi;    // Spherical position
    float dr, dtheta, dphi; // First derivatives w.r.t. affine parameter λ
    float E, L;             // Conserved energy and angular momentum
};

// ─── Physics functions ────────────────────────────────────────────────────────

static Ray initRay(vec3 pos, vec3 dir) {
    Ray ray;
    ray.x = pos.x;  ray.y = pos.y;  ray.z = pos.z;
    ray.r     = length(pos);
    ray.theta = acosf(pos.z / ray.r);
    ray.phi   = atan2f(pos.y, pos.x);

    float st = sinf(ray.theta), ct = cosf(ray.theta);
    float sp = sinf(ray.phi),   cp = cosf(ray.phi);

    ray.dr     =  st*cp*dir.x + st*sp*dir.y + ct*dir.z;
    ray.dtheta = (ct*cp*dir.x + ct*sp*dir.y - st*dir.z) / ray.r;
    ray.dphi   = (-sp*dir.x  + cp*dir.y)               / (ray.r * st);

    ray.L = ray.r * ray.r * st * ray.dphi;

    float f     = 1.0f - SagA_rs / ray.r;
    float dt_dL = sqrtf((ray.dr * ray.dr) / f
                        + ray.r * ray.r * (ray.dtheta * ray.dtheta
                                           + st * st * ray.dphi * ray.dphi));
    ray.E = f * dt_dL;
    return ray;
}

// Returns the first-order derivatives (d1) and second-order derivatives (d2)
// of the Schwarzschild geodesic equations in spherical coordinates.
static void geodesicRHS(const Ray& ray, vec3& d1, vec3& d2) {
    float r      = ray.r,      theta  = ray.theta;
    float dr     = ray.dr,     dtheta = ray.dtheta, dphi = ray.dphi;
    float f      = 1.0f - SagA_rs / r;
    float dt_dL  = ray.E / f;
    float st     = sinf(theta), ct = cosf(theta);

    d1 = vec3(dr, dtheta, dphi);

    d2.x = -(SagA_rs / (2.0f * r * r)) * f  * dt_dL * dt_dL
           + (SagA_rs / (2.0f * r * r * f)) * dr * dr
           + r * (dtheta * dtheta + st * st * dphi * dphi);
    d2.y = -2.0f * dr * dtheta / r + st * ct * dphi * dphi;
    d2.z = -2.0f * dr * dphi   / r - 2.0f * ct / st * dtheta * dphi;
}

// Helper: advance a copy of a ray's spherical state by h * (d1, d2).
// Does NOT reproject to Cartesian (only the final step needs that).
static Ray applyDelta(const Ray& base, const vec3& d1, const vec3& d2, float h) {
    Ray r = base;
    r.r      += h * d1.x;
    r.theta  += h * d1.y;
    r.phi    += h * d1.z;
    r.dr     += h * d2.x;
    r.dtheta += h * d2.y;
    r.dphi   += h * d2.z;
    return r;
}

// True 4-stage Runge-Kutta integrator for the geodesic equations.
// Evaluates geodesicRHS four times per step; fourth-order accurate in D_LAMBDA.
static void rk4Step(Ray& ray, float dL) {
    vec3 k1a, k1b, k2a, k2b, k3a, k3b, k4a, k4b;

    // k1 = f(y_n)
    geodesicRHS(ray, k1a, k1b);
    // k2 = f(y_n + dL/2 * k1)
    geodesicRHS(applyDelta(ray, k1a, k1b, dL * 0.5f), k2a, k2b);
    // k3 = f(y_n + dL/2 * k2)
    geodesicRHS(applyDelta(ray, k2a, k2b, dL * 0.5f), k3a, k3b);
    // k4 = f(y_n + dL * k3)
    geodesicRHS(applyDelta(ray, k3a, k3b, dL),         k4a, k4b);

    // y_{n+1} = y_n + dL/6 * (k1 + 2*k2 + 2*k3 + k4)
    const float c = dL / 6.0f;
    ray.r      += c * (k1a.x + 2.0f*k2a.x + 2.0f*k3a.x + k4a.x);
    ray.theta  += c * (k1a.y + 2.0f*k2a.y + 2.0f*k3a.y + k4a.y);
    ray.phi    += c * (k1a.z + 2.0f*k2a.z + 2.0f*k3a.z + k4a.z);
    ray.dr     += c * (k1b.x + 2.0f*k2b.x + 2.0f*k3b.x + k4b.x);
    ray.dtheta += c * (k1b.y + 2.0f*k2b.y + 2.0f*k3b.y + k4b.y);
    ray.dphi   += c * (k1b.z + 2.0f*k2b.z + 2.0f*k3b.z + k4b.z);

    // Reproject spherical → Cartesian
    float st = sinf(ray.theta), ct = cosf(ray.theta);
    float sp = sinf(ray.phi),   cp = cosf(ray.phi);
    ray.x = ray.r * st * cp;
    ray.y = ray.r * st * sp;
    ray.z = ray.r * ct;
}

static bool crossesEquatorialPlane(vec3 oldPos, vec3 newPos,
                                    float disk_r1, float disk_r2) {
    bool crossed = (oldPos.y * newPos.y < 0.0f);
    float r = sqrtf(newPos.x * newPos.x + newPos.z * newPos.z);
    return crossed && (r >= disk_r1 && r <= disk_r2);
}

// ─── Engine ───────────────────────────────────────────────────────────────────
struct Engine {
    GLFWwindow* window = nullptr;
    GLuint quadVAO = 0, quadVBO = 0;
    GLuint texture = 0;
    GLuint shaderProgram = 0;

    int WIDTH = 800, HEIGHT = 600;
    static constexpr int CW = 200, CH = 150; // ray-trace resolution

    std::vector<uint8_t> pixelBuf;

    Engine() {
        window = WindowManager::CreateWindow(WIDTH, HEIGHT,
                                             "Black Hole – CPU RK4", 3, 3);
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        WIDTH = fbW;  HEIGHT = fbH;

        pixelBuf.assign(CW * CH * 4, 0);

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

    void renderCpu(const Camera& cam) {
        vec3 fwd   = normalize(cam.target - cam.position());
        vec3 up    = vec3(0.0f, 1.0f, 0.0f);
        vec3 right = normalize(cross(fwd, up));
        up         = cross(right, fwd);

        float tanHFov = tanf(glm::radians(60.0f * 0.5f));
        float aspect  = float(WIDTH) / float(HEIGHT);
        vec3  camPos  = cam.position();

        float disk_r1 = float(SagA.r_s * 2.2);
        float disk_r2 = float(SagA.r_s * 5.2);

        const int     W   = CW, H = CH;
        uint8_t*      buf = pixelBuf.data();
        unsigned      nT  = std::max(1u, std::thread::hardware_concurrency());
        int           rpm = (H + int(nT) - 1) / int(nT);

        auto renderBand = [&](int yS, int yE) {
            for (int py = yS; py < yE; ++py) {
                for (int px = 0; px < W; ++px) {
                    float u = (2.0f*(px + 0.5f)/W  - 1.0f) * aspect * tanHFov;
                    float v = (1.0f - 2.0f*(py + 0.5f)/H)  * tanHFov;
                    vec3 dir = normalize(u * right - v * up + fwd);
                    Ray  ray = initRay(camPos, dir);

                    bool hitBH = false, hitDisk = false, hitObj = false;
                    vec4 objColor(0.0f);
                    vec3 objCenter(0.0f);
                    vec3 prevPos(ray.x, ray.y, ray.z);

                    for (int i = 0; i < 60000; ++i) {
                        if (ray.r <= SagA_rs) { hitBH = true; break; }
                        rk4Step(ray, D_LAMBDA);

                        vec3 newPos(ray.x, ray.y, ray.z);
                        if (crossesEquatorialPlane(prevPos, newPos, disk_r1, disk_r2)) {
                            hitDisk = true; break;
                        }
                        for (int j = 0; j < int(objects.size()); ++j) {
                            vec3  c = vec3(objects[j].posRadius);
                            float r = objects[j].posRadius.w;
                            if (glm::distance(newPos, c) <= r) {
                                objColor  = objects[j].color;
                                objCenter = c;
                                hitObj    = true;
                                break;
                            }
                        }
                        if (hitObj) break;
                        prevPos = newPos;
                        if (ray.r > float(ESCAPE_R)) break;
                    }

                    float cr = 0, cg = 0, cb = 0, ca = 0;
                    if (hitDisk) {
                        float rv = length(vec3(ray.x, ray.y, ray.z)) / disk_r2;
                        cr = 1.0f; cg = rv; cb = 0.2f; ca = rv;
                    } else if (hitBH) {
                        ca = 1.0f;
                    } else if (hitObj) {
                        vec3 P = vec3(ray.x, ray.y, ray.z);
                        vec3 N = normalize(P - objCenter);
                        vec3 V = normalize(camPos - P);
                        float ambient   = 0.1f;
                        float diff      = glm::max(dot(N, V), 0.0f);
                        float intensity = ambient + (1.0f - ambient) * diff;
                        cr = objColor.r * intensity;
                        cg = objColor.g * intensity;
                        cb = objColor.b * intensity;
                        ca = objColor.a;
                    }

                    int idx = 4 * (py * W + px);
                    buf[idx+0] = uint8_t(std::min(cr, 1.0f) * 255.0f);
                    buf[idx+1] = uint8_t(std::min(cg, 1.0f) * 255.0f);
                    buf[idx+2] = uint8_t(std::min(cb, 1.0f) * 255.0f);
                    buf[idx+3] = uint8_t(std::min(ca, 1.0f) * 255.0f);
                }
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(nT);
        for (unsigned t = 0; t < nT; ++t) {
            int yS = int(t) * rpm;
            int yE = std::min(yS + rpm, H);
            if (yS >= H) break;
            threads.emplace_back(renderBand, yS, yE);
        }
        for (auto& th : threads) th.join();

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, W, H,
                        GL_RGBA, GL_UNSIGNED_BYTE, buf);
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

Engine* g_engine = nullptr;

int main() {
    Engine eng;
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
        eng.renderCpu(camera);
        eng.drawQuad();

        glfwSwapBuffers(eng.window);
        glfwPollEvents();

        ++framesCount;
        double nowD = chrono::duration<double>(Clock::now().time_since_epoch()).count();
        if (nowD - lastPrintTime >= 1.0) {
            cout << "[CPU-RK4] " << framesCount << " fps  |  "
                 << std::thread::hardware_concurrency() << " threads\n";
            framesCount   = 0;
            lastPrintTime = nowD;
        }
    }

    glfwDestroyWindow(eng.window);
    glfwTerminate();
    return 0;
}
