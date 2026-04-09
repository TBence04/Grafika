#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <fstream>

using namespace std;

// ================= SETTINGS =================
constexpr int WINDOW_WIDTH = 600;
constexpr int WINDOW_HEIGHT = 600;

constexpr int CURVE_SAMPLES = 200;

constexpr float POINT_SIZE = 10.0f;
constexpr float LINE_WIDTH = 3.0f;

constexpr float PICK_RADIUS = 0.005f;

// keys
constexpr int TOGGLE_POLYGON_KEY = GLFW_KEY_P;
constexpr int EXIT_KEY = GLFW_KEY_ESCAPE;


// colors
const glm::vec3 COLOR_BG = { 0.08f, 0.08f, 0.10f };
const glm::vec3 COLOR_CURVE = { 0.0f, 1.0f, 1.0f };
const glm::vec3 COLOR_CTRL = { 1.0f, 1.0f, 0.0f };
const glm::vec3 COLOR_POLY = { 1.0f, 0.0f, 0.0f };

// ================= GLOBALS =================
GLuint program;
GLuint vao, vboCurve, vboPoints, vboPoly;

vector<glm::vec3> ctrl;
vector<glm::vec3> curve;

int selected = -1;
int hovered = -1;

bool dragging = false;
bool showPolygon = false;

int W = WINDOW_WIDTH;
int H = WINDOW_HEIGHT;

// ================= SHADER =================
string readFile(const char* path)
{
    ifstream f(path);
    string s, line;

    while (getline(f, line))
        s += line + "\n";

    return s;
}

GLuint createProgram()
{
    string vs = readFile("vertexShader.glsl");
    string fs = readFile("fragmentShader.glsl");

    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);

    const char* vsrc = vs.c_str();
    const char* fsrc = fs.c_str();

    glShaderSource(v, 1, &vsrc, nullptr);
    glShaderSource(f, 1, &fsrc, nullptr);

    glCompileShader(v);
    glCompileShader(f);

    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    glDeleteShader(v);
    glDeleteShader(f);

    return p;
}

// ================= BEZIER =================
glm::vec3 lerp(glm::vec3 a, glm::vec3 b, float t)
{
    return a + t * (b - a);
}

glm::vec3 deCasteljau(vector<glm::vec3> p, float t)
{
    while (p.size() > 1)
    {
        vector<glm::vec3> n;
        for (int i = 0; i < p.size() - 1; i++)
            n.push_back(lerp(p[i], p[i + 1], t));
        p = n;
    }
    return p[0];
}

void updateCurve()
{
    curve.clear();
    if (ctrl.size() < 2) return;

    for (int i = 0; i <= CURVE_SAMPLES; i++)
        curve.push_back(deCasteljau(ctrl, i / (float)CURVE_SAMPLES));
}

// ================= COORD =================
glm::vec3 screenToWorld(double x, double y)
{
    return {
        (2.0f * x) / W - 1.0f,
        1.0f - (2.0f * y) / H,
        0.0f
    };
}

// ================= HOVER =================
void updateHover(glm::vec3 p)
{
    hovered = -1;

    for (int i = 0; i < ctrl.size(); i++)
    {
        float dx = ctrl[i].x - p.x;
        float dy = ctrl[i].y - p.y;

        if (dx * dx + dy * dy < PICK_RADIUS)
        {
            hovered = i;
            return;
        }
    }
}

// ================= INPUT =================
void mouse(GLFWwindow* w, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double x, y;
        glfwGetCursorPos(w, &x, &y);
        glm::vec3 p = screenToWorld(x, y);

        for (int i = 0; i < ctrl.size(); i++)
        {
            float dx = ctrl[i].x - p.x;
            float dy = ctrl[i].y - p.y;

            if (dx * dx + dy * dy < PICK_RADIUS)
            {
                selected = i;
                dragging = true;
                return;
            }
        }

        ctrl.push_back(p);
        updateCurve();
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        if (hovered != -1 && hovered < ctrl.size())
        {
            ctrl.erase(ctrl.begin() + hovered);
            hovered = -1;
            updateCurve();
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        dragging = false;
        selected = -1;
    }
}

void key(GLFWwindow* window, int key, int, int action, int)
{
    if (action == GLFW_PRESS)
    {
        if (key == TOGGLE_POLYGON_KEY)
        {
            showPolygon = !showPolygon;
        }

        if (key == EXIT_KEY)
        {
            glfwSetWindowShouldClose(window, true);
        }
    }
}

void cursor(GLFWwindow*, double x, double y)
{
    glm::vec3 p = screenToWorld(x, y);

    if (dragging && selected >= 0)
    {
        ctrl[selected] = p;
        updateCurve();
    }

    updateHover(p);
}

// ================= RESIZE =================
void resize(GLFWwindow*, int w, int h)
{
    W = w;
    H = h;
    glViewport(0, 0, W, H);
}

// ================= INIT =================
void init()
{
    program = createProgram();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vboCurve);
    glGenBuffers(1, &vboPoints);
    glGenBuffers(1, &vboPoly);

    ctrl = {
    {-0.6f, -0.6f, 0},
    {-0.6f,  0.8f, 0},
    { 0.6f,  0.8f, 0},
    { 0.6f, -0.6f, 0}
    };

    updateCurve();
}

// ================= DRAW =================
void display()
{
    glClearColor(COLOR_BG.x, COLOR_BG.y, COLOR_BG.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);
    GLint col = glGetUniformLocation(program, "color");

    // CURVE
    glUniform3f(col, COLOR_CURVE.x, COLOR_CURVE.y, COLOR_CURVE.z);

    glLineWidth(LINE_WIDTH);

    glBindBuffer(GL_ARRAY_BUFFER, vboCurve);
    glBufferData(GL_ARRAY_BUFFER, curve.size() * sizeof(glm::vec3), curve.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glDrawArrays(GL_LINE_STRIP, 0, curve.size());

    // CONTROL POINTS
    glUniform3f(col, COLOR_CTRL.x, COLOR_CTRL.y, COLOR_CTRL.z);

    glBindBuffer(GL_ARRAY_BUFFER, vboPoints);
    glBufferData(GL_ARRAY_BUFFER, ctrl.size() * sizeof(glm::vec3), ctrl.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glPointSize(POINT_SIZE);
    glDrawArrays(GL_POINTS, 0, ctrl.size());

    // CONTROL POLYGON
    if (showPolygon && ctrl.size() > 1)
    {
        vector<glm::vec3> lines;

        for (int i = 0; i < ctrl.size() - 1; i++)
        {
            lines.push_back(ctrl[i]);
            lines.push_back(ctrl[i + 1]);
        }

        glUniform3f(col, COLOR_POLY.x, COLOR_POLY.y, COLOR_POLY.z);

        glBindBuffer(GL_ARRAY_BUFFER, vboPoly);
        glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(glm::vec3), lines.data(), GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        glDrawArrays(GL_LINES, 0, lines.size());
    }
}

// ================= MAIN =================
int main()
{
    glfwInit();
    GLFWwindow* w = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Bezier editor", nullptr, nullptr);
    glfwMakeContextCurrent(w);
    glewInit();

    glfwSetMouseButtonCallback(w, mouse);
    glfwSetCursorPosCallback(w, cursor);
    glfwSetKeyCallback(w, key);
    glfwSetFramebufferSizeCallback(w, resize);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    init();

    while (!glfwWindowShouldClose(w))
    {
        display();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }

    glfwTerminate();
}