#include <iostream>

#include <glm/glm.hpp>

#include "Context.h"

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

int main() {
    Context context(700, 500, "Vulkan Framework");
    glfwSetKeyCallback(context.getWindow().get(), keyCallback);

    while(!glfwWindowShouldClose(context.getWindow().get())) {
        glfwPollEvents();

    }
    vkDeviceWaitIdle(context.getDevice());

    context.cleanUp();

    return 0;
}