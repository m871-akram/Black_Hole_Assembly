#ifndef COMMON_HPP
#define COMMON_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace glm;

// ============================================================================
// SHARED CONSTANTS
// ============================================================================
const double SPEED_OF_LIGHT = 299792458.0;
const double GRAVITATIONAL_CONSTANT = 6.67430e-11;

// ============================================================================
// SHADER UTILITIES
// ============================================================================
class ShaderUtils {
public:
    static GLuint CompileShader(const char* source, GLenum type) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader compilation failed: " << infoLog << std::endl;
        }
        return shader;
    }

    static GLuint CreateProgram(const char* vertexSource, const char* fragmentSource) {
        GLuint vertexShader = CompileShader(vertexSource, GL_VERTEX_SHADER);
        GLuint fragmentShader = CompileShader(fragmentSource, GL_FRAGMENT_SHADER);

        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return program;
    }

    static GLuint LoadProgramFromFiles(const char* vertPath, const char* fragPath) {
        auto loadFile = [](const char* path) -> std::string {
            std::ifstream in(path);
            if (!in.is_open()) {
                std::cerr << "Failed to open shader file: " << path << std::endl; // fichier introuvable, t'as oublié de le créer?
                exit(EXIT_FAILURE);
            }
            std::stringstream ss;
            ss << in.rdbuf();
            return ss.str();
        };

        std::string vertSource = loadFile(vertPath);
        std::string fragSource = loadFile(fragPath);

        return CreateProgram(vertSource.c_str(), fragSource.c_str());
    }

    static GLuint LoadComputeShader(const char* path) {
        std::ifstream in(path);
        if (!in.is_open()) {
            std::cerr << "Failed to open compute shader: " << path << std::endl; // pas trouvé le compute shader, cherche mieux mec
            exit(EXIT_FAILURE);
        }
        std::stringstream ss;
        ss << in.rdbuf();
        std::string srcStr = ss.str();
        const char* src = srcStr.c_str();

        GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(cs, 1, &src, nullptr);
        glCompileShader(cs);

        GLint ok;
        glGetShaderiv(cs, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            GLint logLen;
            glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &logLen);
            std::vector<char> log(logLen);
            glGetShaderInfoLog(cs, logLen, nullptr, log.data());
            std::cerr << "Compute shader compile error:\n" << log.data() << std::endl; // erreur de compil dans le compute shader, gg
            exit(EXIT_FAILURE);
        }

        GLuint prog = glCreateProgram();
        glAttachShader(prog, cs);
        glLinkProgram(prog);
        glGetProgramiv(prog, GL_LINK_STATUS, &ok);
        if (!ok) {
            GLint logLen;
            glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
            std::vector<char> log(logLen);
            glGetProgramInfoLog(prog, logLen, nullptr, log.data());
            std::cerr << "Compute shader link error:\n" << log.data() << std::endl; // le link du compute shader a planté, aïe
            exit(EXIT_FAILURE);
        }

        glDeleteShader(cs);
        return prog;
    }
};

// ============================================================================
// GESTIONNAIRE DE FENÊTRE (parce que GLFW c'est relou à setup)
// ============================================================================
class WindowManager {
public:
    static GLFWwindow* CreateWindow(int width, int height, const char* title,
                                     int majorVersion = 3, int minorVersion = 3) {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl; // GLFW veut pas démarrer, redémarre ton ordi mdr
            return nullptr;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, majorVersion);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minorVersion);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl; // la fenêtre veut pas se créer, t'as un problème de config
            glfwTerminate();
            return nullptr;
        }

        glfwMakeContextCurrent(window);

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl; // GLEW a planté, check tes drivers OpenGL
            glfwDestroyWindow(window);
            glfwTerminate();
            return nullptr;
        }

        // callback pour le resize histoire que ça suit quand tu redimensionnes comme un fou
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int w, int h) {
            glViewport(0, 0, w, h);
        });

        // viewport de départ sinon rien s'affiche au début mdr
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);

        return window;
    }
};

// ============================================================================
// QUAD PLEIN ÉCRAN (pour plaquer une texture sur tout l'écran, du classique)
// ============================================================================
class FullScreenQuad {
private:
    GLuint VAO, VBO;
    GLuint shaderProgram;

public:
    FullScreenQuad() {
        const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        })";

        const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D screenTexture;
        void main() {
            FragColor = texture(screenTexture, TexCoord);
        })";

        shaderProgram = ShaderUtils::CreateProgram(vertexShaderSource, fragmentShaderSource);

        float quadVertices[] = {
            // positions   // coords de texture
            -1.0f,  1.0f,  0.0f, 1.0f,  // coin haut gauche
            -1.0f, -1.0f,  0.0f, 0.0f,  // coin bas gauche
             1.0f, -1.0f,  1.0f, 0.0f,  // coin bas droit
            -1.0f,  1.0f,  0.0f, 1.0f,  // coin haut gauche (bis)
             1.0f, -1.0f,  1.0f, 0.0f,  // coin bas droit (bis)
             1.0f,  1.0f,  1.0f, 1.0f   // coin haut droit
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void Draw(GLuint texture) {
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(glGetUniformLocation(shaderProgram, "screenTexture"), 0);

        glDisable(GL_DEPTH_TEST);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEnable(GL_DEPTH_TEST);
    }

    ~FullScreenQuad() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }
};

// ============================================================================
// CAMÉRA ORBITALE (pour tourner autour du bordel qu'on simule)
// ============================================================================
class OrbitCamera {
public:
    vec3 target;
    float radius;
    float minRadius, maxRadius;
    float azimuth;
    float elevation;
    float orbitSpeed;
    float zoomSpeed;
    float fov;

    bool dragging;
    bool panning;
    bool moving;
    double lastX, lastY;

    OrbitCamera(vec3 target = vec3(0.0f), float radius = 15.0f,
                float minRadius = 1.0f, float maxRadius = 1000.0f, float fov = 60.0f)
        : target(target), radius(radius), minRadius(minRadius), maxRadius(maxRadius),
          azimuth(0.0f), elevation(float(M_PI) / 2.0f), orbitSpeed(0.01f),
          zoomSpeed(1.0f), fov(fov), dragging(false), panning(false),
          moving(false), lastX(0.0), lastY(0.0) {}

    vec3 position() const {
        float clampedElevation = glm::clamp(elevation, 0.01f, float(M_PI) - 0.01f);
        return vec3(
            radius * sin(clampedElevation) * cos(azimuth),
            radius * cos(clampedElevation),
            radius * sin(clampedElevation) * sin(azimuth)
        ) + target;
    }

    void update() {
        moving = dragging || panning;
    }

    virtual void processMouseMove(double x, double y) {
        if (!dragging) return;

        float dx = float(x - lastX);
        float dy = float(y - lastY);

        if (panning) {
            // mode pan, on déplace la cible pour se balader
            vec3 forward = normalize(target - position());
            vec3 right = normalize(cross(forward, vec3(0, 1, 0)));
            vec3 up = cross(right, forward);
            float panSpeed = 0.005f * radius;
            target += -right * dx * panSpeed + up * dy * panSpeed;
        } else {
            // mode orbit, on tourne autour comme un satellite paumé
            azimuth += dx * orbitSpeed;
            elevation -= dy * orbitSpeed;
            elevation = glm::clamp(elevation, 0.01f, float(M_PI) - 0.01f);
        }

        lastX = x;
        lastY = y;
        update();
    }

    virtual void processMouseButton(int button, int action, int mods, GLFWwindow* win) {
        if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_MIDDLE) {
            if (action == GLFW_PRESS) {
                dragging = true;
                panning = (mods & GLFW_MOD_SHIFT) || (button == GLFW_MOUSE_BUTTON_MIDDLE);
                glfwGetCursorPos(win, &lastX, &lastY);
            } else if (action == GLFW_RELEASE) {
                dragging = false;
                panning = false;
            }
        }
    }

    virtual void processScroll(double yoffset) {
        radius -= yoffset * zoomSpeed;
        radius = glm::clamp(radius, minRadius, maxRadius);
        update();
    }

    mat4 getViewMatrix() const {
        return lookAt(position(), target, vec3(0, 1, 0));
    }

    mat4 getProjectionMatrix(float aspectRatio, float nearPlane = 0.1f, float farPlane = 1000.0f) const {
        return perspective(radians(fov), aspectRatio, nearPlane, farPlane);
    }

    // wrappers statiques pcq GLFW aime bien les fonctions C old school
    static void RegisterCallbacks(GLFWwindow* window, OrbitCamera* camera) {
        glfwSetWindowUserPointer(window, camera);

        glfwSetMouseButtonCallback(window, [](GLFWwindow* win, int button, int action, int mods) {
            auto* cam = static_cast<OrbitCamera*>(glfwGetWindowUserPointer(win));
            if (cam) cam->processMouseButton(button, action, mods, win);
        });

        glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y) {
            auto* cam = static_cast<OrbitCamera*>(glfwGetWindowUserPointer(win));
            if (cam) cam->processMouseMove(x, y);
        });

        glfwSetScrollCallback(window, [](GLFWwindow* win, double xoffset, double yoffset) {
            auto* cam = static_cast<OrbitCamera*>(glfwGetWindowUserPointer(win));
            if (cam) cam->processScroll(yoffset);
        });
    }
};

// ============================================================================
// GÉNÉRATEUR DE SPHÈRES (pour faire des boules en 3D tranquille)
// ============================================================================
class SphereGenerator {
public:
    static std::vector<float> GenerateVertices(float radius, int stacks = 25, int sectors = 25) {
        std::vector<float> vertices;

        for (float i = 0.0f; i <= stacks; ++i) {
            float theta1 = (i / stacks) * float(M_PI);
            float theta2 = ((i + 1) / stacks) * float(M_PI);

            for (float j = 0.0f; j < sectors; ++j) {
                float phi1 = (j / sectors) * 2.0f * float(M_PI);
                float phi2 = ((j + 1) / sectors) * 2.0f * float(M_PI);

                auto sphericalToCartesian = [](float r, float theta, float phi) -> vec3 {
                    return vec3(
                        r * sin(theta) * cos(phi),
                        r * cos(theta),
                        r * sin(theta) * sin(phi)
                    );
                };

                vec3 v1 = sphericalToCartesian(radius, theta1, phi1);
                vec3 v2 = sphericalToCartesian(radius, theta1, phi2);
                vec3 v3 = sphericalToCartesian(radius, theta2, phi1);
                vec3 v4 = sphericalToCartesian(radius, theta2, phi2);

                // Triangle 1
                vertices.insert(vertices.end(), {v1.x, v1.y, v1.z});
                vertices.insert(vertices.end(), {v2.x, v2.y, v2.z});
                vertices.insert(vertices.end(), {v3.x, v3.y, v3.z});

                // Triangle 2
                vertices.insert(vertices.end(), {v2.x, v2.y, v2.z});
                vertices.insert(vertices.end(), {v4.x, v4.y, v4.z});
                vertices.insert(vertices.end(), {v3.x, v3.y, v3.z});
            }
        }

        return vertices;
    }

    static void CreateVAO(GLuint& VAO, GLuint& VBO, const std::vector<float>& vertices) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }
};

// ============================================================================
// UTILITAIRES PHYSIQUES (les maths qui font mal au crâne)
// ============================================================================
class PhysicsUtils {
public:
    static double CalculateSchwarzschildRadius(double mass) {
        return 2.0 * GRAVITATIONAL_CONSTANT * mass / (SPEED_OF_LIGHT * SPEED_OF_LIGHT); // rayon de Schwarzschild, au-delà c'est le point de non-retour
    }

    static vec3 CalculateGravitationalForce(vec3 pos1, float mass1, vec3 pos2, float mass2) {
        vec3 dir = pos2 - pos1;
        float distance = length(dir);

        if (distance < 0.0001f) return vec3(0.0f); // évite la division par zéro sinon ça pète

        dir = normalize(dir);
        double force = GRAVITATIONAL_CONSTANT * mass1 * mass2 / (distance * distance); // F = G*m1*m2/r^2, merci Isaac
        return dir * float(force / mass1);
    }

    static float CalculateOrbitalVelocity(float centralMass, float distance) {
        return sqrt(GRAVITATIONAL_CONSTANT * centralMass / distance); // vitesse orbitale stable, faut pas aller trop vite ni trop lent
    }
};

#endif // COMMON_HPP
