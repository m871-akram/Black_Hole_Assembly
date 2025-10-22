#version 330 core
layout(location = 0) in vec3 aPos; // la position, classique quoi
uniform mat4 viewProj; // matrice de projection pour pas que tout soit n'importe où
void main() {
    gl_Position = viewProj * vec4(aPos, 1.0); // on balance les coords dans l'espace écran
}
