#include "common.hpp"
#include "physics_asm.hpp"  

// Ouais, ici on balance les shaders de la mort avec des effets de lumière et de glow stylés
const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out float lightIntensity;
void main() {
    // Transformation magique vers l’espace clip, classique mais indispensable
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vec3 worldPos = (model * vec4(aPos, 1.0)).xyz;
    vec3 normal = normalize(aPos);
    vec3 dirToCenter = normalize(-worldPos);
    // Calcul de la lumière, pour faire genre c’est un peu réaliste
    lightIntensity = max(dot(normal, dirToCenter), 0.3);
})glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
in float lightIntensity;
out vec4 FragColor;
uniform vec4 objectColor;
uniform bool isGrid;
uniform bool GLOW;
void main() {
    // Un peu de logique visuelle : si c’est une grille -> blanc, sinon -> glow de l’espace
    if (isGrid) {
        FragColor = objectColor;
    } else if(GLOW){
        // Mode “je brûle la carte graphique” activé
        FragColor = vec4(objectColor.rgb * 10000000, objectColor.a);
    } else {
        // Sinon on fait un petit effet smooth, tranquille
        float fade = smoothstep(0.0, 10.0, lightIntensity*10);
        FragColor = vec4(objectColor.rgb * fade, objectColor.a);
    }
})glsl";

// Variables globales aka les trucs qu’on touche partout (pas top mais bon…)
bool running = true;
bool pause = false;  // On démarre sans pause, faut bien que ça bouge un peu
float deltaTime = 0.0;
float lastFrame = 0.0;

// Constantes physiques — t’inquiète, Einstein valide
const float c = 299792458.0f;
float initMass = float(pow(10, 22));
float sizeRatio = 30000.0f;  // Ce ratio magique qui fait tout marcher “à peu près”

// Caméra : on la fout loin pour voir tout le bazar
OrbitCamera camera(vec3(0.0f, 0.0f, 0.0f), 50000.0f, 1000.0f, 200000.0f, 45.0f);

// Protos de fonctions, histoire que le compilateur arrête de râler
GLFWwindow* StartGLU();
GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource);
void CreateVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount);
void UpdateCam(GLuint shaderProgram);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void DrawGrid(GLuint shaderProgram, GLuint gridVAO, size_t vertexCount);

// Classe Object : ici on gère les planètes, les masses, la physique, bref... le cosmos
class Object {
public:
    GLuint VAO, VBO;
    glm::vec3 position = glm::vec3(400, 300, 0);
    glm::vec3 velocity = glm::vec3(0, 0, 0);
    size_t vertexCount;
    glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

    bool Initalizing = false;
    bool Launched = false;
    bool target = false;
    bool glow = false;

    float mass;
    float density;
    float radius;

    glm::vec3 LastPos = position;

    // Constructeur : on crée une sphère qui flotte dans le vide
    Object(glm::vec3 initPosition, glm::vec3 initVelocity, float mass, float density = 3344,
           glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), bool Glow = false) {
        this->position = initPosition;
        this->velocity = initVelocity;
        this->mass = mass;
        this->density = density;
        // Petit calcul de rayon, un peu barbare mais ça marche
        this->radius = pow(((3 * this->mass/this->density)/(4 * 3.14159265359)), (1.0f/3.0f)) / sizeRatio;
        this->color = color;
        this->glow = Glow;

        std::vector<float> vertices = SphereGenerator::GenerateVertices(this->radius);
        vertexCount = vertices.size();
        SphereGenerator::CreateVAO(VAO, VBO, vertices);
    }

    // Mise à jour de la position, un peu de mouvement sinon c’est boring
    void UpdatePos(){
        this->position[0] += this->velocity[0] / 94;
        this->position[1] += this->velocity[1] / 94;
        this->position[2] += this->velocity[2] / 94;
        this->radius = pow(((3 * this->mass/this->density)/(4 * 3.14159265359)), (1.0f/3.0f)) / sizeRatio;
    }

    // On recalcule la sphère si jamais sa taille change (aka si elle gonfle ou fond)
    void UpdateVertices() {
        std::vector<float> vertices = SphereGenerator::GenerateVertices(this->radius);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    }

    glm::vec3 GetPos() const {
        return this->position;
    }

    // Accélération, c’est là qu’on applique les forces, yeah baby Newton
    void accelerate(float x, float y, float z){
        this->velocity[0] += x / 96;
        this->velocity[1] += y / 96;
        this->velocity[2] += z / 96;
    }

    // Collision ? Spoiler : non, juste un check bidon pour l'instant
    float CheckCollision(const Object& other) {
        // ⚡ ASSEMBLY-OPTIMIZED: Using fast distance calculation
        float distSq = PhysicsASM::DistanceSquared(
            this->position[0], this->position[1], this->position[2],
            other.position[0], other.position[1], other.position[2]
        );
        float distance = std::sqrt(distSq);  // Only sqrt if we really need actual distance
        return 1.0f;
    }
};

// Le grand tableau des objets (aka le système solaire de fortune)
std::vector<Object> objs = {};
std::vector<float> CreateGridVertices(float size, int divisions, const std::vector<Object>& objs);
std::vector<float> UpdateGridVertices(std::vector<float> vertices, const std::vector<Object>& objs, float halfSize, float originalY);

GLuint gridVAO, gridVBO;

// Ici on assemble les shaders comme des LEGO
GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Erreur compilation vertex shader : " << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Erreur compilation fragment shader : " << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Erreur linkage shader program : " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// VBO + VAO setup : le duo infernal
void CreateVBOVAO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t vertexCount) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(float), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

// Mise à jour de la caméra, aka “suivre le bazar en mouvement”
void UpdateCam(GLuint shaderProgram) {
    glUseProgram(shaderProgram);
    glm::vec3 cameraPos = camera.position();
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(cameraPos, camera.target, cameraUp);
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
}

// Les raccourcis clavier du gamer
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) pause = !pause;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwTerminate();
        glfwWindowShouldClose(window);
        running = false;
    }
}

// Souris et autres joyeusetés
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    camera.processMouseMove(xpos, ypos);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods){
    camera.processMouseButton(button, action, mods, window);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
    camera.processScroll(yoffset);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Fonction pour dessiner la grille : le plan de l’univers, tranquille
void DrawGrid(GLuint shaderProgram, GLuint gridVAO, size_t vertexCount) {
    glUseProgram(shaderProgram);
    glm::mat4 model = glm::mat4(1.0f);
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(gridVAO);
    glDrawArrays(GL_LINES, 0, vertexCount / 3);
    glBindVertexArray(0);
}

std::vector<float> CreateGridVertices(float size, int divisions, const std::vector<Object>& objs) {
    std::vector<float> vertices;
    float step = size / divisions;
    float halfSize = size / 2.0f;

    // Lignes selon X — ou comment tracer un joli quadrillage de l’espace-temps
    for (int yStep = 3; yStep <= 3; ++yStep) {
        float y = -halfSize*0.3f + yStep * step;
        for (int zStep = 0; zStep <= divisions; ++zStep) {
            float z = -halfSize + zStep * step;
            for (int xStep = 0; xStep < divisions; ++xStep) {
                float xStart = -halfSize + xStep * step;
                float xEnd = xStart + step;
                vertices.push_back(xStart); vertices.push_back(y); vertices.push_back(z);
                vertices.push_back(xEnd);   vertices.push_back(y); vertices.push_back(z);
            }
        }
    }

    // Lignes selon Z — histoire de croiser le bazar
    for (int xStep = 0; xStep <= divisions; ++xStep) {
        float x = -halfSize + xStep * step;
        for (int yStep = 3; yStep <= 3; ++yStep) {
            float y = -halfSize*0.3f + yStep * step;
            for (int zStep = 0; zStep < divisions; ++zStep) {
                float zStart = -halfSize + zStep * step;
                float zEnd = zStart + step;
                vertices.push_back(x); vertices.push_back(y); vertices.push_back(zStart);
                vertices.push_back(x); vertices.push_back(y); vertices.push_back(zEnd);
            }
        }
    }

    return vertices;
}

// Mise à jour de la grille avec la courbure de l’espace-temps (oui oui, Einstein inside)
std::vector<float> UpdateGridVertices(std::vector<float> vertices, const std::vector<Object>& objs,
                                     float halfSize, float originalY) {
    glm::vec3 corners[4] = {
        glm::vec3(-halfSize, originalY, -halfSize),
        glm::vec3(halfSize, originalY, -halfSize),
        glm::vec3(-halfSize, originalY, halfSize),
        glm::vec3(halfSize, originalY, halfSize)
    };

    float dy_corners[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    // On calcule combien chaque coin se fait "tirer" par la gravité
    for (int i = 0; i < 4; i++) {
        for (const auto& obj : objs) {
            glm::vec3 toObject = obj.GetPos() - corners[i];
            float distance = glm::length(toObject);
            float distance_m = distance * 1000.0f;
            float rs = (2 * GRAVITATIONAL_CONSTANT * obj.mass) / (c * c);
            if (distance_m > rs) {
                float dz = 2 * std::sqrt(rs * (distance_m - rs));
                dy_corners[i] += dz * 2.0f;
            }
        }
    }

    // Maintenant on fait de la magie mathématique : interpolation bilinéaire
    for (size_t i = 0; i < vertices.size(); i += 3) {
        float x = vertices[i];
        float z = vertices[i + 2];

        glm::vec3 vertexPos(x, originalY, z);
        float dy = 0.0f;

        for (const auto& obj : objs) {
            glm::vec3 toObject = obj.GetPos() - vertexPos;
            float distance = glm::length(toObject);
            float distance_m = distance * 1000.0f;
            float rs = (2 * GRAVITATIONAL_CONSTANT * obj.mass) / (c * c);
            if (distance_m > rs) {
                float dz = 2 * std::sqrt(rs * (distance_m - rs));
                dy += dz * 2.0f;
            }
        }

        float u = (x + halfSize) / (2 * halfSize);
        float v = (z + halfSize) / (2 * halfSize);

        float shift = (1 - u) * (1 - v) * dy_corners[0] +
                      u * (1 - v) * dy_corners[1] +
                      (1 - u) * v * dy_corners[2] +
                      u * v * dy_corners[3];

        vertices[i + 1] = originalY + (dy - shift) + halfSize / 3;
    }

    return vertices;
}

// Variables globales d’écran (aka le canvas de ton univers)
int g_fbWidth = 800;
int g_fbHeight = 600;

// MAIN — le cœur du game 💥
int main() {
    GLFWwindow* window = StartGLU();
    GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUseProgram(shaderProgram);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Callback pour resize, sinon l’image devient Picasso quand on change la fenêtre
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int width, int height) {
        g_fbWidth = width;
        g_fbHeight = height;
        glViewport(0, 0, width, height);
    });

    glfwGetFramebufferSize(window, &g_fbWidth, &g_fbHeight);
    glViewport(0, 0, g_fbWidth, g_fbHeight);

    // On crée les planètes et satellites — l’armée du cosmos
    float planetMass = 1.989e25f;
    float satelliteMass = 5.97219e22f * 27.0f;
    float orbitVel = 1500.0f;

    objs = {
        Object(glm::vec3(-8000, 0, 0), glm::vec3(0, 0, 0), planetMass, 5515,
               glm::vec4(0.2f, 0.5f, 0.9f, 1.0f), false),
        Object(glm::vec3(4000, 0, 6000), glm::vec3(0, 0, 0), planetMass, 5515,
               glm::vec4(0.3f, 0.9f, 0.3f, 1.0f), false),
        Object(glm::vec3(4000, 0, -6000), glm::vec3(0, 0, 0), planetMass, 5515,
               glm::vec4(0.9f, 0.3f, 0.3f, 1.0f), false),
        Object(glm::vec3(-11000, 0, 0), glm::vec3(0, 0, orbitVel), satelliteMass, 3344,
               glm::vec4(0.7f, 0.8f, 0.9f, 1.0f), false),
        Object(glm::vec3(-5000, 0, 0), glm::vec3(0, 0, -orbitVel), satelliteMass, 3344,
               glm::vec4(0.6f, 0.7f, 0.85f, 1.0f), false),
        Object(glm::vec3(1000, 0, 6000), glm::vec3(0, 0, orbitVel), satelliteMass, 3344,
               glm::vec4(0.7f, 0.95f, 0.7f, 1.0f), false),
        Object(glm::vec3(4000, 0, -9000), glm::vec3(orbitVel, 0, 0), satelliteMass, 3344,
               glm::vec4(0.95f, 0.7f, 0.7f, 1.0f), false),
        Object(glm::vec3(4000, 0, -1500), glm::vec3(-orbitVel * 0.85f, 0, 0), satelliteMass * 0.7f, 3344,
               glm::vec4(0.9f, 0.6f, 0.6f, 1.0f), false),
    };

    float size = 20000.0f;
    int divisions = 25;
    float step = size / divisions;
    float halfSize = size / 2.0f;
    float originalY = -halfSize * 0.3f + 3 * step;

    std::vector<float> gridVertices = CreateGridVertices(size, divisions, objs);
    CreateVBOVAO(gridVAO, gridVBO, gridVertices.data(), gridVertices.size());

    // BOUCLE PRINCIPALE : aka “la danse des planètes”
    while (!glfwWindowShouldClose(window) && running == true) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        UpdateCam(shaderProgram);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)g_fbWidth / (float)g_fbHeight, 0.1f, 750000.0f);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Grille spatio-temporelle — pour donner le ton interstellaire
        glUseProgram(shaderProgram);
        glUniform4f(objectColorLoc, 1.0f, 1.0f, 1.0f, 0.25f);
        glUniform1i(glGetUniformLocation(shaderProgram, "isGrid"), 1);
        glUniform1i(glGetUniformLocation(shaderProgram, "GLOW"), 0);
        gridVertices = UpdateGridVertices(gridVertices, objs, halfSize, originalY);
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float), gridVertices.data(), GL_DYNAMIC_DRAW);
        DrawGrid(shaderProgram, gridVAO, gridVertices.size());

        // Et maintenant : les objets (aka les planètes qui se tirent dessus avec gravité)
        for(auto& obj : objs) {
            glUniform4f(objectColorLoc, obj.color.r, obj.color.g, obj.color.b, obj.color.a);

            for(auto& obj2 : objs){
                if(&obj2 != &obj && !obj.Initalizing && !obj2.Initalizing){
                   
                    float distSq = PhysicsASM::DistanceSquared(
                        obj.GetPos()[0], obj.GetPos()[1], obj.GetPos()[2],
                        obj2.GetPos()[0], obj2.GetPos()[1], obj2.GetPos()[2]
                    );

                    float distance = std::sqrt(distSq);

                    if (distance > 0) {
                        
                        float direction[3] = {
                            obj2.GetPos()[0] - obj.GetPos()[0],
                            obj2.GetPos()[1] - obj.GetPos()[1],
                            obj2.GetPos()[2] - obj.GetPos()[2]
                        };
                        PhysicsASM::Normalize(direction);  

                        distance *= 1000;

                        
                        float distance_scaled_sq = distance * distance;
                        double Gforce = (GRAVITATIONAL_CONSTANT * obj.mass * obj2.mass) / distance_scaled_sq;

                        float acc1 = Gforce / obj.mass;

                        
                        float acc[3];
                        PhysicsASM::VectorScale(direction, acc1, acc);

                        if(!pause){
                            obj.accelerate(acc[0], acc[1], acc[2]);
                        }

                        obj.velocity *= obj.CheckCollision(obj2);
                    }
                }
            }

            if(!pause){
                obj.UpdatePos();
            }

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, obj.position);
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(glGetUniformLocation(shaderProgram, "isGrid"), 0);
            glUniform1i(glGetUniformLocation(shaderProgram, "GLOW"), obj.glow ? 1 : 0);

            glBindVertexArray(obj.VAO);
            glDrawArrays(GL_TRIANGLES, 0, obj.vertexCount / 3);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Ménage de fin (on libère la RAM avant que le PC chauffe)
    for (auto& obj : objs) {
        glDeleteVertexArrays(1, &obj.VAO);
        glDeleteBuffers(1, &obj.VBO);
    }

    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();

    return 0;
}

// Initialisation d’OpenGL et GLFW : le passage obligé
GLFWwindow* StartGLU() {
    if (!glfwInit()) {
        std::cout << "GLFW a planté avant même de commencer... super." << std::endl;
        return nullptr;
    }
    GLFWwindow* window = glfwCreateWindow(800, 600, "Simulation Gravitationnelle - Projet du Futur", NULL, NULL);
    if (!window) {
        std::cerr << "Pas de fenêtre, pas de simulation. RIP." << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW a dit non. Dommage." << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, 800, 600);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return window;
}
