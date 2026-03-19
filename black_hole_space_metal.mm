// black_hole_space_metal.mm
// Metal compute-shader black hole ray tracer for Apple Silicon (M1/M2/M3).
//
// Pipeline:
//   1. Metal compute kernel (traceGeodesic) traces Schwarzschild geodesics and
//      writes RGBA pixels into an MTLTexture (200×150).
//   2. A Metal render pass (blitVertex + blitFragment) upscales that texture to
//      the full CAMetalLayer drawable using bilinear filtering.
//   3. GLFW owns the Cocoa window for keyboard / mouse input; no OpenGL context
//      is created.
//
// Build target: BlackHole_space_metal  (CMakeLists.txt, Apple Silicon block)

// ── Do NOT pull in any GL/GLEW headers — this is a pure-Metal binary ─────────
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Foundation/Foundation.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <simd/simd.h>

#include <cmath>
#include <iostream>
#include <vector>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace glm;
using namespace std;
using Clock = std::chrono::high_resolution_clock;

// ─────────────────────────────────────────────────────────────────────────────
//  Metal shader source — embedded as a C++ raw string literal.
//  Compiled at runtime via [MTLDevice newLibraryWithSource:options:error:].
//
//  Contains three Metal functions:
//    • traceGeodesic  — compute kernel  (Schwarzschild geodesic integrator)
//    • blitVertex     — vertex function (fullscreen quad, no vertex buffer)
//    • blitFragment   — fragment function (bilinear upscale of compute output)
// ─────────────────────────────────────────────────────────────────────────────
static const char* kMetalSource = R"METAL(
#include <metal_stdlib>
using namespace metal;

// ── Scene constants ──────────────────────────────────────────────────────────
constant float  SagA_rs  = 1.269e10f;
constant float  D_LAMBDA = 1e7f;
constant float  ESCAPE_R = 1e30f;

// ── Uniform buffer layouts ────────────────────────────────────────────────────
// Must match the C++ host structs MetalCameraData / MetalDiskData / MetalObjectsData
// declared in this file (identical field order and sizes).

struct CameraData {
    float4 pos;          // xyz = position, w = padding
    float4 right;
    float4 up;
    float4 fwd;
    float  tanHalfFov;
    float  aspect;
    int    moving;
    int    _p4;
};

struct DiskData {
    float r1;
    float r2;
    float num;          // unused — kept for struct-size parity with host
    float thickness;
};

struct ObjectsData {
    int    numObjects;
    int    _p0, _p1, _p2;
    float4 posRadius[16]; // xyz = centre, w = radius
    float4 color[16];     // rgba
    float  mass[16];      // tightly packed (Metal layout, not std140)
};

// ── Ray ───────────────────────────────────────────────────────────────────────
struct Ray {
    float x, y, z;          // Cartesian (recomputed after each step)
    float r, theta, phi;    // Spherical coordinates
    float dr, dtheta, dphi; // First derivatives w.r.t. affine parameter λ
    float E, L;             // Conserved energy and angular momentum
};

// ── initRay ───────────────────────────────────────────────────────────────────
Ray initRay(float3 pos, float3 dir) {
    Ray ray;
    ray.x = pos.x; ray.y = pos.y; ray.z = pos.z;
    ray.r     = length(pos);
    ray.theta = acos(pos.z / ray.r);
    ray.phi   = atan2(pos.y, pos.x);

    float st = sin(ray.theta), ct = cos(ray.theta);
    float sp = sin(ray.phi),   cp = cos(ray.phi);

    ray.dr     =  st*cp*dir.x + st*sp*dir.y + ct*dir.z;
    ray.dtheta = (ct*cp*dir.x + ct*sp*dir.y - st*dir.z) / ray.r;
    ray.dphi   = (-sp*dir.x   + cp*dir.y)               / (ray.r * st);

    ray.L = ray.r * ray.r * st * ray.dphi;

    float f     = 1.0f - SagA_rs / ray.r;
    float dt_dL = sqrt((ray.dr * ray.dr) / f
                       + ray.r * ray.r * (ray.dtheta * ray.dtheta
                                          + st * st * ray.dphi * ray.dphi));
    ray.E = f * dt_dL;
    return ray;
}

// ── geodesicRHS ───────────────────────────────────────────────────────────────
// Schwarzschild geodesic equations in spherical coordinates.
// d1 = (dr, dθ, dφ), d2 = (d²r, d²θ, d²φ).
void geodesicRHS(Ray ray, thread float3& d1, thread float3& d2) {
    float r      = ray.r,      theta  = ray.theta;
    float dr     = ray.dr,     dtheta = ray.dtheta, dphi = ray.dphi;
    float f      = 1.0f - SagA_rs / r;
    float dt_dL  = ray.E / f;
    float st     = sin(theta), ct = cos(theta);

    d1 = float3(dr, dtheta, dphi);
    d2.x = -(SagA_rs / (2.0f * r * r)) * f  * dt_dL * dt_dL
           + (SagA_rs / (2.0f * r * r * f)) * dr * dr
           + r * (dtheta * dtheta + st * st * dphi * dphi);
    d2.y = -2.0f * dr * dtheta / r + st * ct * dphi * dphi;
    d2.z = -2.0f * dr * dphi   / r - 2.0f * ct / st * dtheta * dphi;
}

// ── eulerStep ─────────────────────────────────────────────────────────────────
// Symplectic Euler step — mirrors the "rk4Step" in geodesic.comp exactly.
void eulerStep(thread Ray& ray, float dL) {
    float3 d1, d2;
    geodesicRHS(ray, d1, d2);

    ray.r      += dL * d1.x;
    ray.theta  += dL * d1.y;
    ray.phi    += dL * d1.z;
    ray.dr     += dL * d2.x;
    ray.dtheta += dL * d2.y;
    ray.dphi   += dL * d2.z;

    float st = sin(ray.theta), ct = cos(ray.theta);
    float sp = sin(ray.phi),   cp = cos(ray.phi);
    ray.x = ray.r * st * cp;
    ray.y = ray.r * st * sp;
    ray.z = ray.r * ct;
}

// ── crossesEquatorialPlane ────────────────────────────────────────────────────
bool crossesEquatorialPlane(float3 oldPos, float3 newPos,
                              float disk_r1, float disk_r2) {
    bool  crossed = (oldPos.y * newPos.y < 0.0f);
    float r       = length(float2(newPos.x, newPos.z));
    return crossed && (r >= disk_r1 && r <= disk_r2);
}

// ── traceGeodesic — compute kernel ───────────────────────────────────────────
kernel void traceGeodesic(
    texture2d<float, access::write> outTex  [[texture(0)]],
    constant CameraData&            cam     [[buffer(1)]],
    constant DiskData&              disk    [[buffer(2)]],
    constant ObjectsData&           objs    [[buffer(3)]],
    uint2 gid [[thread_position_in_grid]])
{
    uint W = outTex.get_width();
    uint H = outTex.get_height();
    if (gid.x >= W || gid.y >= H) return;

    // Pixel → ray direction (same mapping as geodesic.comp main())
    float u = (2.0f * (float(gid.x) + 0.5f) / float(W) - 1.0f)
              * cam.aspect * cam.tanHalfFov;
    float v = (1.0f - 2.0f * (float(gid.y) + 0.5f) / float(H))
              * cam.tanHalfFov;

    float3 right = cam.right.xyz;
    float3 up    = cam.up.xyz;
    float3 fwd   = cam.fwd.xyz;
    float3 dir   = normalize(u * right - v * up + fwd);

    Ray    ray     = initRay(cam.pos.xyz, dir);
    float3 prevPos = float3(ray.x, ray.y, ray.z);

    bool   hitBH   = false, hitDisk = false, hitObj = false;
    float4 objColor  = float4(0);
    float3 objCenter = float3(0);

    for (int i = 0; i < 60000; ++i) {
        if (ray.r <= SagA_rs) { hitBH = true; break; }
        eulerStep(ray, D_LAMBDA);

        float3 newPos = float3(ray.x, ray.y, ray.z);
        if (crossesEquatorialPlane(prevPos, newPos, disk.r1, disk.r2)) {
            hitDisk = true; break;
        }
        for (int j = 0; j < objs.numObjects; ++j) {
            float3 centre = objs.posRadius[j].xyz;
            float  radius = objs.posRadius[j].w;
            if (length(newPos - centre) <= radius) {
                objColor  = objs.color[j];
                objCenter = centre;
                hitObj    = true;
                break;
            }
        }
        if (hitObj) break;
        prevPos = newPos;
        if (ray.r > ESCAPE_R) break;
    }

    float4 color;
    if (hitDisk) {
        float r = length(float3(ray.x, ray.y, ray.z)) / disk.r2;
        color = float4(1.0f, r, 0.2f, r);
    } else if (hitBH) {
        color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    } else if (hitObj) {
        float3 P = float3(ray.x, ray.y, ray.z);
        float3 N = normalize(P - objCenter);
        float3 V = normalize(cam.pos.xyz - P);
        float  ambient   = 0.1f;
        float  diff      = max(dot(N, V), 0.0f);
        float  intensity = ambient + (1.0f - ambient) * diff;
        color = float4(objColor.rgb * intensity, objColor.a);
    } else {
        color = float4(0.0f);
    }

    outTex.write(color, gid);
}

// ── Blit pass — scales the 200×150 compute texture up to the full drawable ───

struct BlitVaryings {
    float4 position [[position]];
    float2 uv;
};

// Fullscreen quad (6 vertices, no vertex buffer).
// NDC:  (-1,-1) = bottom-left,  (1,1) = top-right
// UV:   (0,0)   = top-left of texture (gid.y == 0 row written by kernel)
vertex BlitVaryings blitVertex(uint vid [[vertex_id]]) {
    const float2 pos[6] = {
        {-1,-1}, { 1,-1}, { 1, 1},
        {-1,-1}, { 1, 1}, {-1, 1}
    };
    const float2 uv[6] = {
        {0,1}, {1,1}, {1,0},
        {0,1}, {1,0}, {0,0}
    };
    BlitVaryings out;
    out.position = float4(pos[vid], 0.0f, 1.0f);
    out.uv       = uv[vid];
    return out;
}

fragment float4 blitFragment(BlitVaryings             in  [[stage_in]],
                              texture2d<float, access::sample> tex [[texture(0)]])
{
    constexpr sampler s(filter::linear, address::clamp_to_edge);
    return tex.sample(s, in.uv);
}
)METAL";

// ─────────────────────────────────────────────────────────────────────────────
//  Host-side structs — must match the MSL struct layouts above byte-for-byte.
// ─────────────────────────────────────────────────────────────────────────────

struct MetalCameraData {
    simd_float4 pos;          // 16 bytes
    simd_float4 right;        // 16 bytes
    simd_float4 up;           // 16 bytes
    simd_float4 fwd;          // 16 bytes
    float  tanHalfFov;        //  4 bytes
    float  aspect;            //  4 bytes
    int    moving;            //  4 bytes
    int    _p4;               //  4 bytes
};  // 80 bytes total

struct MetalDiskData {
    float r1, r2, num, thickness;
};  // 16 bytes

struct MetalObjectsData {
    int    numObjects;
    int    _p0, _p1, _p2;
    simd_float4 posRadius[16];
    simd_float4 color[16];
    float  mass[16];
};

// ─────────────────────────────────────────────────────────────────────────────
//  Scene — identical to black_hole_space.cpp
// ─────────────────────────────────────────────────────────────────────────────
static const double GRAVITATIONAL_CONSTANT = 6.67430e-11;

struct ObjectData {
    vec4  posRadius;
    vec4  color;
    float mass;
    vec3  velocity = vec3(0.0f);
};

static double schwarzschildRadius(double mass) {
    static const double c = 299792458.0;
    return 2.0 * GRAVITATIONAL_CONSTANT * mass / (c * c);
}

static const double SagA_mass = 8.54e36;
static const double SagA_rs   = schwarzschildRadius(SagA_mass);

static vector<ObjectData> objects = {
    { vec4(4e11f, 0.0f, 0.0f, 4e10f), vec4(1,1,0,1), 1.98892e30f },
    { vec4(0.0f, 0.0f, 4e11f, 4e10f), vec4(1,0,0,1), 1.98892e30f },
    { vec4(0.0f, 0.0f, 0.0f, float(SagA_rs)), vec4(0,0,0,1), float(SagA_mass) },
};

// ─────────────────────────────────────────────────────────────────────────────
//  Minimal orbit camera (replicated from common.hpp — avoids GLEW dependency)
// ─────────────────────────────────────────────────────────────────────────────
struct OrbitCam {
    vec3  target     = vec3(0.0f);
    float radius     = 6.34194e10f;
    float minRadius  = 1e10f;
    float maxRadius  = 1e12f;
    float azimuth    = 0.0f;
    float elevation  = float(M_PI) / 2.0f;
    float orbitSpeed = 0.01f;
    float zoomSpeed  = 25e9f;
    float fov        = 60.0f;
    bool  dragging   = false;
    bool  panning    = false;
    bool  moving     = false;
    double lastX = 0, lastY = 0;

    vec3 position() const {
        float e = glm::clamp(elevation, 0.01f, float(M_PI) - 0.01f);
        return vec3(radius * sinf(e) * cosf(azimuth),
                    radius * cosf(e),
                    radius * sinf(e) * sinf(azimuth)) + target;
    }

    void onMouseButton(int button, int action, int mods, GLFWwindow* win) {
        if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (action == GLFW_PRESS) {
                dragging = true;
                panning  = (mods & GLFW_MOD_SHIFT) || (button == GLFW_MOUSE_BUTTON_MIDDLE);
                glfwGetCursorPos(win, &lastX, &lastY);
            } else {
                dragging = false;
                panning  = false;
            }
        }
        moving = dragging || panning;
    }

    void onMouseMove(double x, double y) {
        if (!dragging) return;
        float dx = float(x - lastX), dy = float(y - lastY);
        if (panning) {
            vec3 fwd   = normalize(target - position());
            vec3 right = normalize(cross(fwd, vec3(0,1,0)));
            vec3 up    = cross(right, fwd);
            float ps   = 0.005f * radius;
            target    += -right * dx * ps + up * dy * ps;
        } else {
            azimuth   += dx * orbitSpeed;
            elevation -= dy * orbitSpeed;
            elevation  = glm::clamp(elevation, 0.01f, float(M_PI) - 0.01f);
        }
        lastX = x; lastY = y;
        moving = true;
    }

    void onScroll(double yoffset) {
        radius = glm::clamp(radius - float(yoffset) * zoomSpeed, minRadius, maxRadius);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Metal engine
// ─────────────────────────────────────────────────────────────────────────────
struct MetalEngine {
    // Compute resolution — same as original black_hole_space.cpp
    static constexpr int CW = 200, CH = 150;

    GLFWwindow*              window      = nullptr;
    CAMetalLayer*            metalLayer  = nil;
    id<MTLDevice>            device      = nil;
    id<MTLCommandQueue>      cmdQueue    = nil;
    id<MTLComputePipelineState>  tracePipeline = nil;
    id<MTLRenderPipelineState>   blitPipeline  = nil;
    id<MTLTexture>           outTexture  = nil;
    id<MTLBuffer>            camBuf      = nil;
    id<MTLBuffer>            diskBuf     = nil;
    id<MTLBuffer>            objsBuf     = nil;

    int WIDTH = 800, HEIGHT = 600;

    MetalEngine() {
        // ── GLFW window — no OpenGL context ──────────────────────────────────
        if (!glfwInit()) { cerr << "[Metal] glfwInit failed\n"; exit(1); }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // critical for Metal
        window = glfwCreateWindow(WIDTH, HEIGHT, "Black Hole – Metal GPU",
                                   nullptr, nullptr);
        if (!window) { cerr << "[Metal] glfwCreateWindow failed\n"; exit(1); }

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        WIDTH = fbW; HEIGHT = fbH;

        // ── Metal device & command queue ─────────────────────────────────────
        device   = MTLCreateSystemDefaultDevice();
        cmdQueue = [device newCommandQueue];

        // ── Attach CAMetalLayer to the GLFW Cocoa window ──────────────────────
        NSWindow* nsWin = glfwGetCocoaWindow(window);
        metalLayer = [CAMetalLayer layer];
        metalLayer.device      = device;
        metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        metalLayer.drawableSize = CGSizeMake(fbW, fbH);

        nsWin.contentView.layer      = metalLayer;
        nsWin.contentView.wantsLayer = YES;

        // ── Compile shader library at runtime ────────────────────────────────
        NSString* src = [NSString stringWithUTF8String:kMetalSource];
        NSError*  err = nil;
        id<MTLLibrary> lib = [device newLibraryWithSource:src options:nil error:&err];
        if (!lib) {
            cerr << "[Metal] Shader compile error: "
                 << [[err localizedDescription] UTF8String] << "\n";
            exit(1);
        }

        // ── Compute pipeline ──────────────────────────────────────────────────
        id<MTLFunction> traceFunc = [lib newFunctionWithName:@"traceGeodesic"];
        tracePipeline = [device newComputePipelineStateWithFunction:traceFunc error:&err];
        if (!tracePipeline) {
            cerr << "[Metal] Compute pipeline error: "
                 << [[err localizedDescription] UTF8String] << "\n";
            exit(1);
        }

        // ── Blit render pipeline ──────────────────────────────────────────────
        id<MTLFunction> blitVert = [lib newFunctionWithName:@"blitVertex"];
        id<MTLFunction> blitFrag = [lib newFunctionWithName:@"blitFragment"];

        MTLRenderPipelineDescriptor* rpd = [MTLRenderPipelineDescriptor new];
        rpd.vertexFunction   = blitVert;
        rpd.fragmentFunction = blitFrag;
        rpd.colorAttachments[0].pixelFormat = metalLayer.pixelFormat;

        blitPipeline = [device newRenderPipelineStateWithDescriptor:rpd error:&err];
        if (!blitPipeline) {
            cerr << "[Metal] Render pipeline error: "
                 << [[err localizedDescription] UTF8String] << "\n";
            exit(1);
        }

        // ── Compute output texture (200×150, RGBA8) ───────────────────────────
        MTLTextureDescriptor* texDesc =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                               width:CW
                                                              height:CH
                                                           mipmapped:NO];
        texDesc.usage       = MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead;
        texDesc.storageMode = MTLStorageModePrivate;
        outTexture = [device newTextureWithDescriptor:texDesc];

        // ── Uniform buffers ────────────────────────────────────────────────────
        camBuf  = [device newBufferWithLength:sizeof(MetalCameraData)
                                      options:MTLResourceStorageModeShared];
        diskBuf = [device newBufferWithLength:sizeof(MetalDiskData)
                                      options:MTLResourceStorageModeShared];
        objsBuf = [device newBufferWithLength:sizeof(MetalObjectsData)
                                      options:MTLResourceStorageModeShared];
    }

    // Fill the camera buffer from current OrbitCam state.
    void uploadCamera(const OrbitCam& cam) {
        vec3 fwd   = normalize(cam.target - cam.position());
        vec3 up    = vec3(0.0f, 1.0f, 0.0f);
        vec3 right = normalize(cross(fwd, up));
        up         = cross(right, fwd);

        vec3 pos = cam.position();

        auto* d        = static_cast<MetalCameraData*>(camBuf.contents);
        d->pos         = simd_make_float4(pos.x, pos.y, pos.z, 0.0f);
        d->right       = simd_make_float4(right.x, right.y, right.z, 0.0f);
        d->up          = simd_make_float4(up.x, up.y, up.z, 0.0f);
        d->fwd         = simd_make_float4(fwd.x, fwd.y, fwd.z, 0.0f);
        d->tanHalfFov  = tanf(glm::radians(60.0f * 0.5f));
        d->aspect      = float(WIDTH) / float(HEIGHT);
        d->moving      = cam.moving ? 1 : 0;
        d->_p4         = 0;
    }

    void uploadDisk() {
        auto* d     = static_cast<MetalDiskData*>(diskBuf.contents);
        d->r1        = float(SagA_rs * 2.2);
        d->r2        = float(SagA_rs * 5.2);
        d->num       = 2.0f;
        d->thickness = 1e9f;
    }

    void uploadObjects() {
        auto* d = static_cast<MetalObjectsData*>(objsBuf.contents);
        size_t n = std::min(objects.size(), size_t(16));
        d->numObjects = int(n);
        d->_p0 = d->_p1 = d->_p2 = 0;
        for (size_t i = 0; i < n; ++i) {
            d->posRadius[i] = simd_make_float4(objects[i].posRadius.x,
                                               objects[i].posRadius.y,
                                               objects[i].posRadius.z,
                                               objects[i].posRadius.w);
            d->color[i]     = simd_make_float4(objects[i].color.x,
                                               objects[i].color.y,
                                               objects[i].color.z,
                                               objects[i].color.w);
            d->mass[i]      = objects[i].mass;
        }
    }

    void renderFrame(const OrbitCam& cam) {
        id<CAMetalDrawable> drawable = [metalLayer nextDrawable];
        if (!drawable) return;

        uploadCamera(cam);
        uploadDisk();
        uploadObjects();

        id<MTLCommandBuffer> cmdBuf = [cmdQueue commandBuffer];

        // ── 1. Compute pass: trace geodesics → outTexture ─────────────────────
        {
            id<MTLComputeCommandEncoder> enc =
                [cmdBuf computeCommandEncoderWithDispatchType:MTLDispatchTypeSerial];
            [enc setComputePipelineState:tracePipeline];
            [enc setTexture:outTexture atIndex:0];
            [enc setBuffer:camBuf  offset:0 atIndex:1];
            [enc setBuffer:diskBuf offset:0 atIndex:2];
            [enc setBuffer:objsBuf offset:0 atIndex:3];

            // 16×16 thread groups — matches geodesic.comp local_size_x/y = 16
            MTLSize tgSize  = MTLSizeMake(16, 16, 1);
            MTLSize grid    = MTLSizeMake(CW, CH, 1);

            // dispatchThreads handles non-uniform threadgroups natively on M1.
            if ([device supportsFamily:MTLGPUFamilyApple4]) {
                [enc dispatchThreads:grid threadsPerThreadgroup:tgSize];
            } else {
                // Fallback: round up to full threadgroups
                MTLSize tg = MTLSizeMake((CW + 15)/16, (CH + 15)/16, 1);
                [enc dispatchThreadgroups:tg threadsPerThreadgroup:tgSize];
            }
            [enc endEncoding];
        }

        // ── 2. Render pass: blit outTexture → drawable (bilinear upscale) ────
        {
            MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor new];
            rpd.colorAttachments[0].texture     = drawable.texture;
            rpd.colorAttachments[0].loadAction  = MTLLoadActionClear;
            rpd.colorAttachments[0].clearColor  = MTLClearColorMake(0, 0, 0, 1);
            rpd.colorAttachments[0].storeAction = MTLStoreActionStore;

            id<MTLRenderCommandEncoder> enc =
                [cmdBuf renderCommandEncoderWithDescriptor:rpd];
            [enc setRenderPipelineState:blitPipeline];
            [enc setFragmentTexture:outTexture atIndex:0];
            [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
            [enc endEncoding];
        }

        [cmdBuf presentDrawable:drawable];
        [cmdBuf commit];
    }

    void onResize(int w, int h) {
        WIDTH  = w;
        HEIGHT = h;
        metalLayer.drawableSize = CGSizeMake(w, h);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Globals for callbacks
// ─────────────────────────────────────────────────────────────────────────────
static MetalEngine* g_engine   = nullptr;
static OrbitCam*    g_cam      = nullptr;
static bool         g_gravity  = false;
static double       g_lastPrint = 0;
static int          g_frames   = 0;

int main() {
    MetalEngine eng;
    g_engine = &eng;

    OrbitCam cam;
    g_cam = &cam;

    // ── GLFW callbacks ────────────────────────────────────────────────────────
    glfwSetWindowUserPointer(eng.window, &cam);

    glfwSetMouseButtonCallback(eng.window,
        [](GLFWwindow* win, int button, int action, int mods) {
            auto* c = static_cast<OrbitCam*>(glfwGetWindowUserPointer(win));
            c->onMouseButton(button, action, mods, win);
        });

    glfwSetCursorPosCallback(eng.window,
        [](GLFWwindow* win, double x, double y) {
            auto* c = static_cast<OrbitCam*>(glfwGetWindowUserPointer(win));
            c->onMouseMove(x, y);
        });

    glfwSetScrollCallback(eng.window,
        [](GLFWwindow* win, double, double yo) {
            auto* c = static_cast<OrbitCam*>(glfwGetWindowUserPointer(win));
            c->onScroll(yo);
        });

    glfwSetKeyCallback(eng.window,
        [](GLFWwindow*, int key, int, int action, int) {
            if (action == GLFW_PRESS && key == GLFW_KEY_G) {
                g_gravity = !g_gravity;
                cout << "[Metal] Gravity " << (g_gravity ? "ON" : "OFF") << "\n";
            }
        });

    glfwSetFramebufferSizeCallback(eng.window,
        [](GLFWwindow*, int w, int h) {
            g_engine->onResize(w, h);
        });

    auto   t0        = Clock::now();
    g_lastPrint      = chrono::duration<double>(t0.time_since_epoch()).count();
    double lastTime  = glfwGetTime();

    while (!glfwWindowShouldClose(eng.window)) {
        double now = glfwGetTime();
        double dt  = now - lastTime;
        lastTime   = now;

        // N-body gravity (same logic as original)
        for (auto& obj : objects) {
            for (auto& obj2 : objects) {
                if (&obj == &obj2) continue;
                float dx   = obj2.posRadius.x - obj.posRadius.x;
                float dy   = obj2.posRadius.y - obj.posRadius.y;
                float dz   = obj2.posRadius.z - obj.posRadius.z;
                float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                if (dist > 0 && g_gravity) {
                    vec3   dir    = normalize(vec3(dx, dy, dz));
                    double Gforce = (GRAVITATIONAL_CONSTANT * obj.mass * obj2.mass)
                                    / (double(dist) * double(dist));
                    double acc1   = Gforce / obj.mass;
                    vec3   acc    = dir * float(acc1);
                    obj.velocity    += acc * float(dt);
                    obj.posRadius.x += obj.velocity.x * float(dt);
                    obj.posRadius.y += obj.velocity.y * float(dt);
                    obj.posRadius.z += obj.velocity.z * float(dt);
                }
            }
        }

        eng.renderFrame(cam);
        glfwPollEvents();

        ++g_frames;
        double nowD = chrono::duration<double>(Clock::now().time_since_epoch()).count();
        if (nowD - g_lastPrint >= 1.0) {
            cout << "[Metal GPU] " << g_frames << " fps\n";
            g_frames    = 0;
            g_lastPrint = nowD;
        }
    }

    glfwDestroyWindow(eng.window);
    glfwTerminate();
    return 0;
}
