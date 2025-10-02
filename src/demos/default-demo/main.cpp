#include <iostream>

#include "Context.h"
#include "Camera.h"
#include "StandardRenderers.h"

#include "ResourceLoader.h"

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
    camera->setPosition(glm::vec3(0.0f, 0.3f, 0.0f));

    scene = std::make_shared<Scene>();

    auto modelNode = std::make_unique<SceneNode>();
    ResourceLoader::loadModel("bottle", modelNode);
    modelNode->getChildren().back().get()->setPosition(glm::vec3(0.01f, 0.0f, -0.15f));
    modelNode->getChildren().back().get()->scale(0.1f);
    ResourceLoader::loadModel("teapot", modelNode);
    modelNode->getChildren().back().get()->setPosition(glm::vec3(0.23f, 0.0f, 0.3f));
    modelNode->getChildren().back().get()->rotate(190.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    modelNode->getChildren().back().get()->scale(0.1f);
    scene->addSceneNode(modelNode);
    scene->addSun(30.0f, 50.0f, glm::vec3(0.85f, 0.67f, 0.29f), 1.0f);

    std::vector<glm::vec3> lightPositions = {
        glm::vec3(0.0f, 0.0f, -0.75f),
        glm::vec3(-0.5f, 0.0f, 1.3f),
        glm::vec3(2.0f, 0.5f, 0.0f)
    };
    std::vector<glm::vec3> lightColors = {
        glm::vec3(1.0f, 1.0f, 1.0f),
        glm::vec3(0.4f, 0.78f, 1.0f),
        glm::vec3(1.0f, 0.63f, 0.3f)
    };
    auto lightsNode = std::make_unique<SceneNode>();
    for(int l=0; l<3; l++) {
        auto sceneNode = std::make_unique<SceneNode>();
        auto light = std::make_unique<Light>(lightPositions[l], glm::normalize(-lightPositions[l]));
        light->setColor(lightColors[l]);
        sceneNode->addLight(light);
        lightsNode->addChild(sceneNode);
    }
    scene->addSceneNode(lightsNode);

    ForwardRenderer renderer(context, camera, scene);

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