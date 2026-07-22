#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

struct Point2D {
    float x;
    float y;
};

static std::vector<Point2D> generateBaseOctant(std::int32_t radius) {
    std::vector<Point2D> points;

    if (radius <= 0) {
        return points;
    }

    // Midpoint circle algorithm, restricted to x >= y:
    // one octant from 0 degrees through 45 degrees.
    std::int32_t x = radius;
    std::int32_t y = 0;
    std::int32_t decision = 1 - radius;

    points.reserve(static_cast<std::size_t>(radius * 0.72f) + 2U);

    while (y <= x) {
        points.push_back({
            static_cast<float>(x),
            static_cast<float>(y)
        });

        ++y;

        if (decision < 0) {
            decision += 2 * y + 1;
        } else {
            --x;
            decision += 2 * (y - x) + 1;
        }
    }

    return points;
}

static const char* kVertexShader = R"GLSL(
#version 330 core

layout(location = 0) in vec2 aBasePoint;

uniform vec2  uCenterPixels;
uniform vec2  uViewportPixels;
uniform float uScale;
uniform float uPointSize;

/*
    Homogeneous point vector:

        P = [x, y, 1]^T

    R[i] selects one of the eight circle symmetries.
    T translates the reflected/rotated point to the circle center.
    TR = T * R[i].

    Every base-octant point is submitted once.
    glDrawArraysInstanced(..., 8) creates all eight octants in one draw call.
*/

const mat3 R[8] = mat3[8](
    // 0: ( x,  y)
    mat3(
         1.0,  0.0, 0.0,
         0.0,  1.0, 0.0,
         0.0,  0.0, 1.0
    ),

    // 1: ( y,  x)
    mat3(
         0.0,  1.0, 0.0,
         1.0,  0.0, 0.0,
         0.0,  0.0, 1.0
    ),

    // 2: (-y,  x)
    mat3(
         0.0,  1.0, 0.0,
        -1.0,  0.0, 0.0,
         0.0,  0.0, 1.0
    ),

    // 3: (-x,  y)
    mat3(
        -1.0,  0.0, 0.0,
         0.0,  1.0, 0.0,
         0.0,  0.0, 1.0
    ),

    // 4: (-x, -y)
    mat3(
        -1.0,  0.0, 0.0,
         0.0, -1.0, 0.0,
         0.0,  0.0, 1.0
    ),

    // 5: (-y, -x)
    mat3(
         0.0, -1.0, 0.0,
        -1.0,  0.0, 0.0,
         0.0,  0.0, 1.0
    ),

    // 6: ( y, -x)
    mat3(
         0.0, -1.0, 0.0,
         1.0,  0.0, 0.0,
         0.0,  0.0, 1.0
    ),

    // 7: ( x, -y)
    mat3(
         1.0,  0.0, 0.0,
         0.0, -1.0, 0.0,
         0.0,  0.0, 1.0
    )
);

void main() {
    vec3 P = vec3(aBasePoint * uScale, 1.0);

    mat3 T = mat3(
        1.0,            0.0,            0.0,
        0.0,            1.0,            0.0,
        uCenterPixels.x, uCenterPixels.y, 1.0
    );

    mat3 TR = T * R[gl_InstanceID];
    vec3 pixelPosition = TR * P;

    vec2 ndc;
    ndc.x =  2.0 * pixelPosition.x / uViewportPixels.x - 1.0;
    ndc.y =  1.0 - 2.0 * pixelPosition.y / uViewportPixels.y;

    gl_Position = vec4(ndc, 0.0, 1.0);
    gl_PointSize = uPointSize;
}
)GLSL";

static const char* kFragmentShader = R"GLSL(
#version 330 core

out vec4 FragColor;

void main() {
    // Circularize each GL point instead of displaying a square point.
    vec2 delta = gl_PointCoord - vec2(0.5);
    if (dot(delta, delta) > 0.25) {
        discard;
    }

    FragColor = vec4(1.0);
}
)GLSL";

static void printShaderLog(GLuint shader, const char* label) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length > 1) {
        std::string log(static_cast<std::size_t>(length), '\0');
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        std::cerr << label << " shader log:\n" << log << '\n';
    }
}

static GLuint compileShader(GLenum type, const char* source, const char* label) {
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        throw std::runtime_error("glCreateShader failed");
    }

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    printShaderLog(shader, label);

    if (compiled != GL_TRUE) {
        glDeleteShader(shader);
        throw std::runtime_error(std::string(label) + " shader compilation failed");
    }

    return shader;
}

static GLuint createProgram() {
    const GLuint vertexShader =
        compileShader(GL_VERTEX_SHADER, kVertexShader, "Vertex");

    const GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, kFragmentShader, "Fragment");

    const GLuint program = glCreateProgram();
    if (program == 0) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        throw std::runtime_error("glCreateProgram failed");
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if (length > 1) {
        std::string log(static_cast<std::size_t>(length), '\0');
        glGetProgramInfoLog(program, length, nullptr, log.data());
        std::cerr << "Program link log:\n" << log << '\n';
    }

    if (linked != GL_TRUE) {
        glDeleteProgram(program);
        throw std::runtime_error("Shader program linking failed");
    }

    return program;
}

static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, std::max(width, 1), std::max(height, 1));
}

static void keyCallback(
    GLFWwindow* window,
    int key,
    int,
    int action,
    int
) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main(int argc, char** argv) {
    std::int32_t radius = 250;

    if (argc >= 2) {
        try {
            radius = std::max<std::int32_t>(1, std::stoi(argv[1]));
        } catch (...) {
            std::cerr << "Invalid radius; using 250.\n";
            radius = 250;
        }
    }

    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "GLFW initialization failed.\n";
        return EXIT_FAILURE;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(
        1000,
        800,
        "GPU Eight-Octant Symmetry Circle",
        nullptr,
        nullptr
    );

    if (window == nullptr) {
        std::cerr << "GLFW window creation failed.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    const GLenum glewStatus = glewInit();

    // GLEW may generate GL_INVALID_ENUM in core profile during initialization.
    glGetError();

    if (glewStatus != GLEW_OK) {
        std::cerr << "GLEW initialization failed: "
                  << glewGetErrorString(glewStatus) << '\n';
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    framebufferSizeCallback(window, framebufferWidth, framebufferHeight);

    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;

    try {
        program = createProgram();

        const std::vector<Point2D> baseOctant =
            generateBaseOctant(radius);

        if (baseOctant.empty()) {
            throw std::runtime_error("Base octant generation produced no points");
        }

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                baseOctant.size() * sizeof(Point2D)
            ),
            baseOctant.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            2,
            GL_FLOAT,
            GL_FALSE,
            static_cast<GLsizei>(sizeof(Point2D)),
            reinterpret_cast<void*>(0)
        );

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        const GLint centerLocation =
            glGetUniformLocation(program, "uCenterPixels");
        const GLint viewportLocation =
            glGetUniformLocation(program, "uViewportPixels");
        const GLint scaleLocation =
            glGetUniformLocation(program, "uScale");
        const GLint pointSizeLocation =
            glGetUniformLocation(program, "uPointSize");

        glEnable(GL_PROGRAM_POINT_SIZE);

        std::cout
            << "Radius: " << radius << '\n'
            << "Base-octant points uploaded: " << baseOctant.size() << '\n'
            << "GPU instances per draw: 8\n"
            << "Potential emitted points before boundary duplicates: "
            << baseOctant.size() * 8U << '\n';

        while (glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();

            glfwGetFramebufferSize(
                window,
                &framebufferWidth,
                &framebufferHeight
            );

            framebufferWidth = std::max(framebufferWidth, 1);
            framebufferHeight = std::max(framebufferHeight, 1);

            glViewport(0, 0, framebufferWidth, framebufferHeight);
            glClearColor(0.04f, 0.05f, 0.07f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            glUseProgram(program);

            const float centerX = framebufferWidth * 0.5f;
            const float centerY = framebufferHeight * 0.5f;

            glUniform2f(centerLocation, centerX, centerY);
            glUniform2f(
                viewportLocation,
                static_cast<float>(framebufferWidth),
                static_cast<float>(framebufferHeight)
            );
            glUniform1f(scaleLocation, 1.0f);
            glUniform1f(pointSizeLocation, 3.0f);

            glBindVertexArray(vao);

            /*
                One draw call:
                - baseOctant.size() vertices
                - eight instances
                - gl_InstanceID selects R[0] through R[7]
                - all eight octants are dispatched together
            */
            glDrawArraysInstanced(
                GL_POINTS,
                0,
                static_cast<GLsizei>(baseOctant.size()),
                8
            );

            glBindVertexArray(0);
            glfwSwapBuffers(window);
        }
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';

        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
        }
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
        }
        if (program != 0) {
            glDeleteProgram(program);
        }

        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
