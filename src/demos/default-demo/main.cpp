#include <iostream>

#include "Context.h"
#include "Renderer.h"

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

std::shared_ptr<Context> context = nullptr;

int main() {
    context = std::make_shared<Context>(700, 500, "Vulkan Framework");
    glfwSetKeyCallback(context->getWindow().get(), keyCallback);

    SimpleRenderer renderer(context);

    while(!glfwWindowShouldClose(context->getWindow().get())) {
        glfwPollEvents();

        renderer.update();
        renderer.render();
    }
    vkDeviceWaitIdle(context->getDevice());

    renderer.cleanUp();
    context->cleanUp();

    return 0;
}