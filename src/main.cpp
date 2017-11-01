#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <vector>
#include <memory>

#include "Graphics/GLHelper.h"
#include "Graphics/Mesh.h"
#include "Graphics/GLShaderProgram.h"

#include "Input/GLFWHandler.h"
#include "Input/Keyboard.h"
#include "Input/Mouse.h"

#include "Overlay.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define OPENGL_VERSION_MAJOR 4
#define OPENGL_VERSION_MINOR 1

#define WIDTH 1600
#define HEIGHT 900
#define TITLE "Voxelize"

#ifndef RESOURCE_DIR
#define RESOURCE_DIR "../resources/"
#endif

#ifndef SHADER_DIR
#define SHADER_DIR "../shaders/"
#endif

using namespace std;
using namespace glm;

class Camera {
public:
    Camera() {}

    void update(float dt) {
        float move_speed = speed * dt;
        if (Keyboard::getKeyDown(GLFW_KEY_W))
            position += move_speed * front;
        if (Keyboard::getKeyDown(GLFW_KEY_S))
            position -= move_speed * front;
        if (Keyboard::getKeyDown(GLFW_KEY_A))
            position -= move_speed * glm::normalize(glm::cross(front, up));
        if (Keyboard::getKeyDown(GLFW_KEY_D))
            position += move_speed * glm::normalize(glm::cross(front, up));

        float xoffset = sensitivity * Mouse::getDeltaX();
        float yoffset = sensitivity * Mouse::getDeltaY();

        pitch = glm::clamp(pitch + yoffset, -89.0f, 89.0f);
        yaw = yaw + xoffset;

        front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
        front.y = sin(glm::radians(pitch));
        front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    }

    glm::mat4 lookAt() const { return glm::lookAt(position, position + front, up); }

    glm::vec3 position {0.0f, 0.0f, 0.0f};
    glm::vec3 front {0.0f, 0.0f, -1.0f};
    glm::vec3 up {0.0f, 1.0f, 0.0f};

    float pitch = 0.0f;
    float yaw = -90.0f;

    float fov = 45.0f;

    const float speed = 10.0f;
    const float sensitivity = 0.05f;
};

GLFWwindow *init_window(unsigned width, unsigned height, const char *title);

GLFWwindow *window = nullptr;

Camera camera;
unique_ptr<Mesh> mesh = nullptr;
GLShaderProgram program;

void init() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    glClearColor(0.2f, 0, 0, 1);
    glEnable(GL_DEPTH_TEST);

//    mesh = make_unique<Mesh>(RESOURCE_DIR "cubes.obj");
    mesh = make_unique<Mesh>(RESOURCE_DIR "sponza/sponza_small.obj");

    program.linkProgram(SHADER_DIR "simple.vert", SHADER_DIR "phong.frag");

    camera.position = vec3(0, 0, 5);
}

void update(float dt) {
    Mouse::update();

    if (GLFW_PRESS == Keyboard::getKey(GLFW_KEY_ESCAPE)) {
        glfwSetWindowShouldClose(window, 1);
    }
    else if (GLFW_PRESS == Keyboard::getKey(GLFW_KEY_LEFT_CONTROL)) {
        int mode = glfwGetInputMode(window, GLFW_CURSOR);
        glfwSetInputMode(window, GLFW_CURSOR, (mode == GLFW_CURSOR_DISABLED) ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
    else if (GLFW_PRESS == Keyboard::getKey(GLFW_KEY_GRAVE_ACCENT)) {
        /* TODO: toggle ui */
    }

    camera.update(dt);
}

void render(float dt) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 projection = perspective(camera.fov, (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    mat4 view = camera.lookAt();
    mat4 model;

    vec3 lightPos {100.0f, 100.0f, 0.0f};
    vec3 lightInt {1.0f, 1.0f, 1.0f};

    glEnable(GL_DEPTH_TEST);

    program.bind();
    glUniformMatrix4fv(program.uniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(program.uniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(program.uniformLocation("model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(program.uniformLocation("eye"), 1, glm::value_ptr(camera.position));
    glUniform3fv(program.uniformLocation("lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(program.uniformLocation("lightInt"), 1, glm::value_ptr(lightInt));
    glUniform1i(program.uniformLocation("texture0"), 0);

    mesh->draw();
    program.unbind();
}

int main()
{
    window = init_window(WIDTH, HEIGHT, TITLE);

    init();

    Overlay ui {window};

    float dt, lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        update(dt);
        render(dt);

        ui.render(dt);

        glfwSwapBuffers(window);
    }

    glfwTerminate();

    return 0;
}

GLFWwindow *init_window(unsigned width, unsigned height, const char *title) {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_VERSION_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, GLFWHandler::key_callback);
    glfwSetCursorPosCallback(window, GLFWHandler::mouse_callback);
    glfwSetMouseButtonCallback(window, GLFWHandler::mousebtn_callback);
    glfwSetScrollCallback(window, GLFWHandler::scroll_callback);
    glfwSetCharCallback(window, GLFWHandler::char_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        exit(EXIT_FAILURE);
    }

    GLHelper::printGLInfo();

#ifndef NDEBUG
    GLHelper::registerDebugOutputCallback();
#endif

    return window;
}
