#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <fstream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

#define numVAOs 2

GLuint vao[numVAOs];
GLuint vbo[numVAOs];
GLuint programCircle;
GLuint programLine;

// Kör mozgás
float circleX = 0.0f;
float circleY = 0.0f;
float circleRadiusNorm = 50.0f / 300.0f;

// Irányvektor
const float angleDeg = 25.0f;
const float speedPixels = 1.0f;
const float pixelNorm = 1.0f / 300.0f;

// KEZDŐ: vízszintes mozgás
float circleSpeedX = speedPixels * pixelNorm;
float circleSpeedY = 0.0f;

// Vonal mozgás
float lineY = 0.0f;
float lineStep = 0.02f;

bool upPressed = false;
bool downPressed = false;

// Shader betöltés
string readShaderSource(const char* filePath) {
    string content;
    ifstream file(filePath);
    string line;
    while (getline(file, line)) content += line + "\n";
    return content;
}

GLuint createShaderProgram(const char* vertPath, const char* fragPath) {
    string vertStr = readShaderSource(vertPath);
    string fragStr = readShaderSource(fragPath);

    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

    const char* vertSrc = vertStr.c_str();
    const char* fragSrc = fragStr.c_str();

    glShaderSource(vertShader, 1, &vertSrc, NULL);
    glShaderSource(fragShader, 1, &fragSrc, NULL);

    glCompileShader(vertShader);
    glCompileShader(fragShader);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    return program;
}

// Billentyűk kezelése
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (key == GLFW_KEY_UP) upPressed = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_DOWN) downPressed = (action != GLFW_RELEASE);

    // S -> irányváltás 25 fokra
    if (key == GLFW_KEY_S && action == GLFW_PRESS) {
        float angleRad = angleDeg * (float)M_PI / 180.0f;
        circleSpeedX = speedPixels * cos(angleRad) * pixelNorm;
        circleSpeedY = speedPixels * sin(angleRad) * pixelNorm;
    }
}

// Inicializálás
void init(GLFWwindow* window) {
    programCircle = createShaderProgram("vertexShaderCircle.glsl", "fragmentShaderCircle.glsl");
    programLine = createShaderProgram("vertexShaderLine.glsl", "fragmentShaderLine.glsl");

    glGenVertexArrays(numVAOs, vao);

    // --- KÖR ---
    glBindVertexArray(vao[0]);
    float quadVerts[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };
    glGenBuffers(1, &vbo[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    // --- VONAL ---
    glBindVertexArray(vao[1]);
    float lineWidthNorm = 2.0f / 3.0f;
    float halfWidth = lineWidthNorm / 2.0f;
    float halfHeight = 3.0f / 600.0f;
    float lineVerts[] = {
        -halfWidth, -halfHeight,
         halfWidth, -halfHeight,
        -halfWidth,  halfHeight,
         halfWidth,  halfHeight
    };
    glGenBuffers(1, &vbo[1]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVerts), lineVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

// Precíz kör–téglalap ütközés
bool checkCircleRectCollision(float cx, float cy, float r,
    float rectX, float rectY, float halfWidth, float halfHeight)
{
    // Legközelebbi pont a téglalaphoz
    float closestX = max(rectX - halfWidth, min(cx, rectX + halfWidth));
    float closestY = max(rectY - halfHeight, min(cy, rectY + halfHeight));

    // Távolság a kör középpontjától
    float distX = cx - closestX;
    float distY = cy - closestY;

    return (distX * distX + distY * distY) < (r * r);
}

// Rajzolás
void display(GLFWwindow* window) {
    glClearColor(1.0f, 1.0f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // --- MOZGÁS ---
    circleX += circleSpeedX;
    circleY += circleSpeedY;

    // Falak
    if (circleX + circleRadiusNorm >= 1.0f || circleX - circleRadiusNorm <= -1.0f) {
        circleSpeedX *= -1.0f;
        if (circleX + circleRadiusNorm > 1.0f) circleX = 1.0f - circleRadiusNorm;
        if (circleX - circleRadiusNorm < -1.0f) circleX = -1.0f + circleRadiusNorm;
    }

    if (circleY + circleRadiusNorm >= 1.0f || circleY - circleRadiusNorm <= -1.0f) {
        circleSpeedY *= -1.0f;
        if (circleY + circleRadiusNorm > 1.0f) circleY = 1.0f - circleRadiusNorm;
        if (circleY - circleRadiusNorm < -1.0f) circleY = -1.0f + circleRadiusNorm;
    }

    // Ütközés vonallal
    float halfLineWidth = 2.0f / 3.0f / 2.0f;
    float halfLineHeight = 3.0f / 600.0f;
    bool intersects = checkCircleRectCollision(circleX, circleY, circleRadiusNorm,
        0.0f, lineY, halfLineWidth, halfLineHeight);

    // --- KÖR ---
    glUseProgram(programCircle);
    glBindVertexArray(vao[0]);

    GLint centerLoc = glGetUniformLocation(programCircle, "circleCenter");
    GLint radiusLoc = glGetUniformLocation(programCircle, "radius");
    GLint centerColorLoc = glGetUniformLocation(programCircle, "centerColor");
    GLint edgeColorLoc = glGetUniformLocation(programCircle, "edgeColor");

    if (!intersects) {
        glUniform3f(centerColorLoc, 0.0f, 1.0f, 0.0f);
        glUniform3f(edgeColorLoc, 1.0f, 0.0f, 0.0f);
    }
    else {
        glUniform3f(centerColorLoc, 1.0f, 0.0f, 0.0f);
        glUniform3f(edgeColorLoc, 0.0f, 1.0f, 0.0f);
    }

    glUniform2f(centerLoc, circleX, circleY);
    glUniform1f(radiusLoc, circleRadiusNorm);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // --- VONAL MOZGÁS ---
    if (upPressed && lineY + halfLineHeight < 1.0f) lineY += lineStep;
    if (downPressed && lineY - halfLineHeight > -1.0f) lineY -= lineStep;

    // --- VONAL ---
    glUseProgram(programLine);
    glBindVertexArray(vao[1]);

    GLint lineYLoc = glGetUniformLocation(programLine, "lineY");
    GLint lineColorLoc = glGetUniformLocation(programLine, "lineColor");

    glUniform1f(lineYLoc, lineY);
    glUniform3f(lineColorLoc, 0.0f, 0.0f, 1.0f);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Cleanup
void cleanUpScene() {
    glDeleteProgram(programCircle);
    glDeleteProgram(programLine);
    glDeleteBuffers(numVAOs, vbo);
    glDeleteVertexArrays(numVAOs, vao);
}

// Main
int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow* window = glfwCreateWindow(600, 600, "Bouncing Circle", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);

    if (glewInit() != GLEW_OK) return -1;

    init(window);
    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanUpScene();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}