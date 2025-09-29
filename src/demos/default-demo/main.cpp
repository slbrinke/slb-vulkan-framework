#include <iostream>

#include "Context.h"
#include "Camera.h"
#include "Renderer.h"

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

int screenWidth = 700;
int screenHeight = 500;

std::shared_ptr<Context> context = nullptr;
std::shared_ptr<Camera> camera = nullptr;
std::shared_ptr<Scene> scene = nullptr;

int main() {
    context = std::make_shared<Context>(screenWidth, screenHeight, "Vulkan Framework");
    glfwSetKeyCallback(context->getWindow().get(), keyCallback);

    camera = std::make_shared<Camera>(screenWidth, screenHeight, context->getWindow());

    scene = std::make_shared<Scene>();

    SimpleRenderer renderer(context, camera, scene);

    while(!glfwWindowShouldClose(context->getWindow().get())) {
        glfwPollEvents();
        camera->updateInput(context->getWindow());

        renderer.update();
        renderer.render();
    }
    vkDeviceWaitIdle(context->getDevice());

    renderer.cleanUp();
    scene->cleanUp(context);
    context->cleanUp();

    return 0;
}