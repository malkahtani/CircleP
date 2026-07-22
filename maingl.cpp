// main.cpp
// Build example:
// g++ main.cpp -lglfw -lGLEW -lGL -O3 -o circle_symmetry

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>

struct Point {
    float x;
    float y;
};

std::vector<Point> generateOctant(int r) {
    std::vector<Point> pts;

    int x = r;
    int y = 0;
    int d = 1 - r;

    while (x >= y) {
        pts.push_back({(float)x, (float)y});

        y++;

        if (d < 0) {
            d += 2 * y + 1;
        } else {
            x--;
            d += 2 * (y - x) + 1;
        }
    }

    return pts;
}

const char* vertexShaderSrc = R"(
#version 330 core

layout(location = 0) in vec2 octantPoint;

uniform vec2 center;
uniform float scale;
uniform vec2 screenSize;

vec2 reflectPoint(vec2 p, int id) {
    float x = p.x;
    float y = p.y;

    if (id == 0) return vec2( x,  y);
    if (id == 1) return vec2( y,  x);
    if (id == 2) return vec2(-y,  x);
    if (id == 3) return vec2(-x,  y);
    if (id == 4) return vec2(-x, -y);
    if (id == 5) return vec2(-y, -x);
    if (id == 6) return vec2( y, -x);
    return vec2( x, -y);
}

void main() {
    vec2 p = reflectPoint(octantPoint, gl_InstanceID);
    vec2 pixel = center + p * scale;

    vec2 ndc;
    ndc.x =  2.0 * pixel.x / screenSize.x - 1.0;
    ndc.y =  1.0 - 2.0 * pixel.y / screenSize.y;

    gl_Position = vec4(ndc, 0.0, 1.0);
    gl_PointSize = 2.0;
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
)";

GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << log << std::endl;
    }

    return shader;
}

GLuint createProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

int main() {
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(900, 700, "Symmetric Circle OpenGL", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glewInit();

    int radius = 200;
    std::vector<Point> octant = generateOctant(radius);

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, octant.size() * sizeof(Point), octant.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
    glEnableVertexAttribArray(0);

    GLuint program = createProgram();

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);

        glUniform2f(glGetUniformLocation(program, "center"), 450.0f, 350.0f);
        glUniform1f(glGetUniformLocation(program, "scale"), 1.0f);
        glUniform2f(glGetUniformLocation(program, "screenSize"), 900.0f, 700.0f);

        glBindVertexArray(vao);

        // Draw one octant, instanced 8 times for 8-way symmetry.
        glDrawArraysInstanced(GL_POINTS, 0, (GLsizei)octant.size(), 8);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
