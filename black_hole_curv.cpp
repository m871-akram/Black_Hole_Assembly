#include "common.hpp"
#include "physics_asm.hpp"  // Assembly-optimized physics functions

// déclarations forward pcq sinon le compilo rage
struct Ray;
struct BlackHole;
struct Engine;

// constantes physiques, genre hyper importantes sinon tout pète
const double G = GRAVITATIONAL_CONSTANT;
const double c = SPEED_OF_LIGHT;

// variables globales pcq flemme de refactoriser proprement mdr
std::vector<Ray> rays;
Engine* enginePtr = nullptr;

// le trou noir, attention ça aspire tout
struct BlackHole {
    glm::vec3 position;
    double mass;
    double radius;
    double r_s; // rayon de Schwarzschild

    BlackHole(glm::vec3 pos, float m) : position(pos), mass(m) {
        r_s = 2.0 * G * mass / (c * c); // formule du rayon critique
    }

    void draw() {
        glBegin(GL_TRIANGLE_FAN);
        glColor3f(1.0f, 0.0f, 0.0f);  // rouge genre ça pète bien
        glVertex2f(0.0f, 0.0f);       // le centre du chaos
        for(int i = 0; i <= 100; i++) {
            float angle = 2.0f * M_PI * i / 100;
            float x = r_s * cos(angle);
            float y = r_s * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
    }
};

BlackHole SagA(glm::vec3(0.0f, 0.0f, 0.0f), 8.54e36f); // Sagittarius A*, le monstre au centre de notre galaxie

// rayons de lumière qui se font courber grave par la gravité
struct Ray {
    // coords cartésiennes, tranquille
    double x, y;
    // coords polaires pour faire stylé
    double r, phi;
    double dr, dphi;
    std::vector<glm::vec2> trail; // la trainée qu'on laisse derrière nous
    double E, L;                   // grandeurs conservées (normalement)

    Ray(glm::vec2 pos, glm::vec2 dir) : x(pos.x), y(pos.y) {
        // ⚡ ASSEMBLY-OPTIMIZED: Fast distance calculation
        float distSq = PhysicsASM::DistanceSquared(x, y, 0.0f, 0.0f, 0.0f, 0.0f);
        this->r = std::sqrt(distSq);  // conversion en polaire
        this->phi = atan2(y, x);

        // initialisation des vitesses
        dr = dir.x * cos(phi) + dir.y * sin(phi);
        dphi = (-dir.x * sin(phi) + dir.y * cos(phi)) / r;

        // quantités conservées selon la relativité générale
        L = r*r * dphi;
        double f = 1.0 - SagA.r_s/r;
        double dt_dλ = sqrt((dr*dr)/(f*f) + (r*r*dphi*dphi)/f);
        E = f * dt_dλ;

        // on commence la traînée
        trail.push_back(glm::vec2(x, y));
    }

    void draw() {
        // dessine la position actuelle
        glPointSize(2.0f);
        glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_POINTS);
        glVertex2f(x, y);
        glEnd();

        // dessine la traînée avec fade out progressif
        size_t N = trail.size();
        if (N >= 2) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glLineWidth(1.0f);

            glBegin(GL_LINE_STRIP);
            for (size_t i = 0; i < N; ++i) {
                // alpha qui augmente au fil du temps
                float alpha = float(i) / float(N - 1);
                glColor4f(1.0f, 1.0f, 1.0f, std::max(alpha, 0.05f));
                glVertex2f(trail[i].x, trail[i].y);
            }
            glEnd();

            glDisable(GL_BLEND);
        }
    }

    void step(double dλ, double rs);
};

// moteur de rendu, c'est là que la magie opère
struct Engine {
    GLFWwindow* window;
    int WIDTH = 800;
    int HEIGHT = 600;
    float width = 100000000000.0f;  // largeur de la vue en mètres, ouais c'est énorme
    float height = 75000000000.0f;  // pareil pour la hauteur

    // variables pour la navigation (pan & zoom comme un pro)
    float offsetX = 0.0f, offsetY = 0.0f;
    float zoom = 1.0f;
    bool middleMousePressed = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;

    Engine() {
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl; // GLFW veut pas démarrer, rip
            exit(EXIT_FAILURE);
        }
        window = glfwCreateWindow(WIDTH, HEIGHT, "Black Hole Simulation", NULL, NULL);
        if (!window) {
            std::cerr << "Failed to create GLFW window" << std::endl; // fenêtre pas créée, check tes configs
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
        glfwMakeContextCurrent(window);
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW" << std::endl; // GLEW a planté, vérifie tes drivers
            glfwDestroyWindow(window);
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        // récup la vraie taille du framebuffer (parfois c'est pas celle qu'on demande lol)
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        WIDTH = fbWidth;
        HEIGHT = fbHeight;
        glViewport(0, 0, WIDTH, HEIGHT);

        setupProjection();
    }

    void setupProjection() {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float left = -width / 2.0f + offsetX;
        float right = width / 2.0f + offsetX;
        float bottom = -height / 2.0f + offsetY;
        float top = height / 2.0f + offsetY;
        glOrtho(left, right, bottom, top, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    void run() {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    ~Engine() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

// fonctions physiques, accrochez-vous les maths arrivent
void geodesicRHS(const Ray& ray, double rhs[4], double rs) {
    double r = ray.r;
    double dr = ray.dr;
    double dphi = ray.dphi;
    double E = ray.E;

    double f = 1.0 - rs/r; // facteur métrique de Schwarzschild

    // dérivées première, tranquille
    rhs[0] = dr;
    rhs[1] = dphi;

    // dérivée seconde de r selon les géodésiques nulles (équation de taré)
    double dt_dλ = E / f;
    rhs[2] =
        - (rs/(2*r*r)) * f * (dt_dλ*dt_dλ)
        + (rs/(2*r*r*f)) * (dr*dr)
        + (r - rs) * (dphi*dphi);

    // dérivée seconde de phi
    rhs[3] = -2.0 * dr * dphi / r;
}

void addState(const double a[4], const double b[4], double factor, double out[4]) {
    for (int i = 0; i < 4; i++)
        out[i] = a[i] + b[i] * factor;
}

void rk4Step(Ray& ray, double dλ, double rs) {
    double y0[4] = { ray.r, ray.phi, ray.dr, ray.dphi };
    double k1[4], k2[4], k3[4], k4[4], temp[4];

    geodesicRHS(ray, k1, rs);
    addState(y0, k1, dλ/2.0, temp);
    Ray r2 = ray; r2.r=temp[0]; r2.phi=temp[1]; r2.dr=temp[2]; r2.dphi=temp[3];
    geodesicRHS(r2, k2, rs);

    addState(y0, k2, dλ/2.0, temp);
    Ray r3 = ray; r3.r=temp[0]; r3.phi=temp[1]; r3.dr=temp[2]; r3.dphi=temp[3];
    geodesicRHS(r3, k3, rs);

    addState(y0, k3, dλ, temp);
    Ray r4 = ray; r4.r=temp[0]; r4.phi=temp[1]; r4.dr=temp[2]; r4.dphi=temp[3];
    geodesicRHS(r4, k4, rs);

    ray.r    += (dλ/6.0)*(k1[0] + 2*k2[0] + 2*k3[0] + k4[0]);
    ray.phi  += (dλ/6.0)*(k1[1] + 2*k2[1] + 2*k3[1] + k4[1]);
    ray.dr   += (dλ/6.0)*(k1[2] + 2*k2[2] + 2*k3[2] + k4[2]);
    ray.dphi += (dλ/6.0)*(k1[3] + 2*k2[3] + 2*k3[3] + k4[3]);
}

void Ray::step(double dλ, double rs) {
    // 1) on intègre (r,φ,dr,dφ)
    if(r <= rs) return; // on arrête si on est dans l'horizon des événements (RIP le rayon)
    rk4Step(*this, dλ, rs);

    // 2) on reconvertit en coordonnées cartésiennes x,y
    x = r * cos(phi);
    y = r * sin(phi);

    // 3) on enregistre la traînée
    trail.push_back(glm::vec2(float(x), float(y)));
}

// Fonction principale (let's gooo)
int main() {
    Engine engine;
    enginePtr = &engine;

    // Gestion du redimensionnement de la fenêtre (au cas où t'aurais envie de resize)
    glfwSetFramebufferSizeCallback(engine.window, [](GLFWwindow* win, int width, int height) {
        if (enginePtr) {
            enginePtr->WIDTH = width;
            enginePtr->HEIGHT = height;
            glViewport(0, 0, width, height);
            enginePtr->setupProjection();
        }
    });

    // On initialise les rayons à différentes hauteurs pour voir la lentille gravitationnelle (trop stylé)
    double startX = -8e10;  // Position de départ (côté gauche)
    int numRays = 15;       // Nombre de rayons de lumière
    double spacing = 1e10;  // Espacement vertical entre les rayons

    for (int i = 0; i < numRays; i++) {
        double offsetY = (i - numRays/2.0) * spacing;  // On centre les rayons verticalement
        rays.push_back(Ray(glm::vec2(startX, offsetY), glm::vec2(c, 0.0)));
    }

    while(!glfwWindowShouldClose(engine.window)) {
        engine.run();
        SagA.draw();

        for (auto& ray : rays) {
            ray.step(1.0, SagA.r_s);
            ray.draw();
        }

        glfwSwapBuffers(engine.window);
        glfwPollEvents();
    }

    return 0;
}

