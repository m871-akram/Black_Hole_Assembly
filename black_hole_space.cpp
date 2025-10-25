#include "common.hpp"
#include "physics_asm.hpp"  // Assembly-optimized physics functions
#define _USE_MATH_DEFINES
#include <chrono>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
using namespace std;
using Clock = std::chrono::high_resolution_clock;

// variables globales pcq j'ai la flemme de tout passer en paramètre mdr
double lastPrintTime = 0.0;
int    framesCount   = 0;
struct Ray;
bool Gravity = false;

struct Camera : public OrbitCamera {
    Camera() : OrbitCamera(vec3(0.0f), 6.34194e10f, 1e10f, 1e12f, 60.0f) {
        elevation = M_PI / 2.0f;
        orbitSpeed = 0.01f;
        zoomSpeed = 25e9f;
    }

    void processKey(int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS && key == GLFW_KEY_G) {
            Gravity = !Gravity;
            cout << "[INFO] Gravité " << (Gravity ? "ACTIVÉE" : "DÉSACTIVÉE") << endl; // toggle la gravité avec G, pratique
        }
    }
};
Camera camera;

struct BlackHole {
    vec3 position;
    double mass;
    double radius;
    double r_s; // rayon de Schwarzschild, le point critique quoi

    BlackHole(vec3 pos, float m) : position(pos), mass(m) {
        r_s = PhysicsUtils::CalculateSchwarzschildRadius(mass); // on calcule le rayon critique
    }

    bool Intercept(float px, float py, float pz) const {
        // ⚡ ASSEMBLY-OPTIMIZED: Fast squared distance check (no sqrt needed!)
        double dist2 = PhysicsASM::DistanceSquared(
            px, py, pz,
            float(position.x), float(position.y), float(position.z)
        );
        return dist2 < r_s * r_s; // check si on est tombé dedans, si oui c'est rip
    }
};
BlackHole SagA(vec3(0.0f, 0.0f, 0.0f), 8.54e36); // Sagittarius A*, le bestiau au centre de notre galaxie

struct ObjectData {
    vec4 posRadius; // position xyz + rayon
    vec4 color; // couleur rgba
    float mass; // masse de l'objet
    vec3 velocity = vec3(0.0f, 0.0f, 0.0f); // vitesse initiale
};
vector<ObjectData> objects = {
    { vec4(4e11f, 0.0f, 0.0f, 4e10f), vec4(1,1,0,1), 1.98892e30 }, // étoile jaune
    { vec4(0.0f, 0.0f, 4e11f, 4e10f), vec4(1,0,0,1), 1.98892e30 }, // étoile rouge
    { vec4(0.0f, 0.0f, 0.0f, SagA.r_s), vec4(0,0,0,1), static_cast<float>(SagA.mass) }, // le trou noir au centre
};

struct Engine {
    GLuint gridShaderProgram;
    GLFWwindow* window;
    GLuint quadVAO;
    GLuint texture;
    GLuint shaderProgram;
    GLuint computeProgram = 0;
    GLuint cameraUBO = 0;
    GLuint diskUBO = 0;
    GLuint objectsUBO = 0;
    GLuint gridVAO = 0;
    GLuint gridVBO = 0;
    GLuint gridEBO = 0;
    int gridIndexCount = 0;

    int WIDTH = 800;
    int HEIGHT = 600;
    int COMPUTE_WIDTH = 200;
    int COMPUTE_HEIGHT = 150;
    float width = 100000000000.0f;
    float height = 75000000000.0f;

    Engine() {
        window = WindowManager::CreateWindow(WIDTH, HEIGHT, "Black Hole", 4, 3); // on crée la fenêtre, version OpenGL 4.3

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight); // récup la vraie taille du framebuffer
        WIDTH = fbWidth;
        HEIGHT = fbHeight;

        this->shaderProgram = ShaderUtils::CreateProgram(
            R"(#version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoord;
            void main() {
                gl_Position = vec4(aPos, 0.0, 1.0);
                TexCoord = aTexCoord;
            })",
            R"(#version 330 core
            in vec2 TexCoord;
            out vec4 FragColor;
            uniform sampler2D screenTexture;
            void main() {
                FragColor = texture(screenTexture, TexCoord);
            })"
        ); // shader de base pour afficher une texture sur un quad

        gridShaderProgram = ShaderUtils::LoadProgramFromFiles("grid.vert", "grid.frag"); // shader pour la grille
        computeProgram = ShaderUtils::LoadComputeShader("geodesic.comp"); // compute shader pour les géodésiques

        glGenBuffers(1, &cameraUBO); // UBO pour les données de caméra
        glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
        glBufferData(GL_UNIFORM_BUFFER, 128, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, cameraUBO);

        glGenBuffers(1, &diskUBO); // UBO pour le disque d'accrétion
        glBindBuffer(GL_UNIFORM_BUFFER, diskUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * 4, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, diskUBO);

        glGenBuffers(1, &objectsUBO); // UBO pour tous les objets de la scène
        glBindBuffer(GL_UNIFORM_BUFFER, objectsUBO);
        GLsizeiptr objUBOSize = sizeof(int) + 3 * sizeof(float)
            + 16 * (sizeof(vec4) + sizeof(vec4))
            + 16 * sizeof(float);
        glBufferData(GL_UNIFORM_BUFFER, objUBOSize, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 3, objectsUBO);

        auto result = QuadVAO();
        this->quadVAO = result[0];
        this->texture = result[1];
    }

    void generateGrid(const vector<ObjectData>& objects) {
        const int gridSize = 25; // taille de la grille, pas trop gros sinon ça lag
        const float spacing = 1e10f; // espacement entre les points

        vector<vec3> vertices;
        vector<GLuint> indices;

        for (int z = 0; z <= gridSize; ++z) {
            for (int x = 0; x <= gridSize; ++x) {
                float worldX = (x - gridSize / 2) * spacing;
                float worldZ = (z - gridSize / 2) * spacing;
                float y = 0.0f;

                // on déforme la grille avec la courbure de Schwarzschild, c'est là que ça devient stylé
                for (const auto& obj : objects) {
                    vec3 objPos = vec3(obj.posRadius);
                    double mass = obj.mass;
                    double r_s = PhysicsUtils::CalculateSchwarzschildRadius(mass);
                    double dx = worldX - objPos.x;
                    double dz = worldZ - objPos.z;
                    double dist = sqrt(dx * dx + dz * dz);

                    // évite les racines carrées négatives, sinon le compilo panique
                    if (dist > r_s) {
                        // formule du diagramme d'embedding, les maths de la relativité générale quoi
                        double deltaY = 2.0 * sqrt(r_s * (dist - r_s));
                        y += static_cast<float>(deltaY) - 3e10f; // offset pour bien voir la déformation sinon c'est trop plat
                    } else {
                        // si on est à l'intérieur du rayon de Schwarzschild, ça plonge à fond
                        y += 2.0f * static_cast<float>(sqrt(r_s * r_s)) - 3e10f;
                    }
                }
                vertices.push_back(vec3(worldX, y, worldZ));
            }
        }

        // génération des indices pour dessiner les lignes de la grille
        for (int z = 0; z < gridSize; ++z) {
            for (int x = 0; x < gridSize; ++x) {
                int row1 = z * (gridSize + 1);
                int row2 = (z + 1) * (gridSize + 1);
                indices.push_back(row1 + x);
                indices.push_back(row1 + x + 1);
                indices.push_back(row1 + x);
                indices.push_back(row2 + x);
            }
        }

        if (gridVAO == 0) glGenVertexArrays(1, &gridVAO);
        if (gridVBO == 0) glGenBuffers(1, &gridVBO);
        if (gridEBO == 0) glGenBuffers(1, &gridEBO);

        glBindVertexArray(gridVAO);
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec3), vertices.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gridEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_DYNAMIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

        gridIndexCount = indices.size();
        glBindVertexArray(0);
    }

    void drawGrid(const mat4& viewProj) {
        glUseProgram(gridShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(gridShaderProgram, "viewProj"),
                        1, GL_FALSE, glm::value_ptr(viewProj));
        glBindVertexArray(gridVAO);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDrawElements(GL_LINES, gridIndexCount, GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }

    void drawFullScreenQuad() {
        glUseProgram(shaderProgram);
        glBindVertexArray(quadVAO);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(shaderProgram, "screenTexture"), 0);

        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
        glEnable(GL_DEPTH_TEST);
    }

    void dispatchCompute(const Camera& cam) {
        // determine target compute‐res - use consistent resolution
        int cw = cam.moving ? COMPUTE_WIDTH  : 200;
        int ch = cam.moving ? COMPUTE_HEIGHT : 150;

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, cw, ch, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glUseProgram(computeProgram);
        uploadCameraUBO(cam);
        uploadDiskUBO();
        uploadObjectsUBO(objects);

        glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

        GLuint groupsX = (GLuint)std::ceil(cw / 16.0f);
        GLuint groupsY = (GLuint)std::ceil(ch / 16.0f);
        glDispatchCompute(groupsX, groupsY, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    void uploadCameraUBO(const Camera& cam) {
        struct UBOData {
            vec3 pos; float _pad0;
            vec3 right; float _pad1;
            vec3 up; float _pad2;
            vec3 forward; float _pad3;
            float tanHalfFov;
            float aspect;
            bool moving;
            int _pad4;
        } data;

        vec3 fwd = normalize(cam.target - cam.position());
        vec3 up = vec3(0, 1, 0);
        vec3 right = normalize(cross(fwd, up));
        up = cross(right, fwd);

        data.pos = cam.position();
        data.right = right;
        data.up = up;
        data.forward = fwd;
        data.tanHalfFov = tan(radians(60.0f * 0.5f));
        data.aspect = float(WIDTH) / float(HEIGHT);
        data.moving = cam.dragging || cam.panning;

        glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UBOData), &data);
    }

    void uploadObjectsUBO(const vector<ObjectData>& objs) {
        struct UBOData {
            int numObjects;
            float _pad0, _pad1, _pad2;
            vec4 posRadius[16];
            vec4 color[16];
            float mass[16];
        } data;

        size_t count = std::min(objs.size(), size_t(16));
        data.numObjects = static_cast<int>(count);

        for (size_t i = 0; i < count; ++i) {
            data.posRadius[i] = objs[i].posRadius;
            data.color[i] = objs[i].color;
            data.mass[i] = objs[i].mass;
        }

        glBindBuffer(GL_UNIFORM_BUFFER, objectsUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
    }

    void uploadDiskUBO() {
        float r1 = SagA.r_s * 2.2f;
        float r2 = SagA.r_s * 5.2f;
        float num = 2.0;
        float thickness = 1e9f;
        float diskData[4] = { r1, r2, num, thickness };

        glBindBuffer(GL_UNIFORM_BUFFER, diskUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(diskData), diskData);
    }

    vector<GLuint> QuadVAO() {
        float quadVertices[] = {
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        GLuint VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, COMPUTE_WIDTH, COMPUTE_HEIGHT,
                    0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        return {VAO, texture};
    }
};
Engine engine;

int main() {
    OrbitCamera::RegisterCallbacks(engine.window, &camera);

    glfwSetKeyCallback(engine.window, [](GLFWwindow* win, int key, int scancode, int action, int mods) {
        camera.processKey(key, scancode, action, mods);
    });

    // Gestion du redimensionnement de fenêtre (au cas où tu veux resize pendant que ça tourne)
    glfwSetFramebufferSizeCallback(engine.window, [](GLFWwindow* win, int width, int height) {
        engine.WIDTH = width;
        engine.HEIGHT = height;
        glViewport(0, 0, width, height);
    });

    auto t0 = Clock::now();
    lastPrintTime = chrono::duration<double>(t0.time_since_epoch()).count();
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(engine.window)) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double now = glfwGetTime();
        double dt = now - lastTime;
        lastTime = now;

        // Simulation de la gravité (les corps s'attirent comme moi et mon lit le matin)
        for (auto& obj : objects) {
            for (auto& obj2 : objects) {
                if (&obj == &obj2) continue;

                float dx = obj2.posRadius.x - obj.posRadius.x;
                float dy = obj2.posRadius.y - obj.posRadius.y;
                float dz = obj2.posRadius.z - obj.posRadius.z;
                float distance = sqrt(dx * dx + dy * dy + dz * dz);

                if (distance > 0 && Gravity) {
                    vec3 direction = normalize(vec3(dx, dy, dz));
                    double Gforce = (GRAVITATIONAL_CONSTANT * obj.mass * obj2.mass) / (distance * distance);
                    double acc1 = Gforce / obj.mass;
                    vec3 acc = direction * float(acc1);

                    obj.velocity += acc * float(dt);
                    obj.posRadius.x += obj.velocity.x * float(dt);
                    obj.posRadius.y += obj.velocity.y * float(dt);
                    obj.posRadius.z += obj.velocity.z * float(dt);
                }
            }
        }

        // Grille (avec la courbure de l'espace-temps stylée)
        engine.generateGrid(objects);
        mat4 view = lookAt(camera.position(), camera.target, vec3(0,1,0));
        mat4 proj = perspective(radians(60.0f), float(engine.WIDTH)/float(engine.HEIGHT), 1e9f, 1e14f);
        mat4 viewProj = proj * view;
        engine.drawGrid(viewProj);

        // Raytracer (pour rendre tout ça magnifique)
        glViewport(0, 0, engine.WIDTH, engine.HEIGHT);
        engine.dispatchCompute(camera);
        engine.drawFullScreenQuad();

        glfwSwapBuffers(engine.window);
        glfwPollEvents();
    }

    glfwDestroyWindow(engine.window);
    glfwTerminate();
    return 0;
}

