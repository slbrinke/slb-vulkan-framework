#include <iostream>

#include "Context.h"
#include "Camera.h"
#include "StandardRenderers.h"
#include "CustomRenderers.h"

#include "ResourceLoader.h"

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
}

int screenWidth = 1000;
int screenHeight = 700;

std::shared_ptr<Context> context = nullptr;
std::shared_ptr<Camera> camera = nullptr;
std::shared_ptr<Scene> scene = nullptr;

int main() {
    context = std::make_shared<Context>(screenWidth, screenHeight, "Vulkan Framework");
    glfwSetKeyCallback(context->getWindow().get(), keyCallback);

    camera = std::make_shared<Camera>(screenWidth, screenHeight, context->getWindow());
    camera->setPosition(glm::vec3(0.0f, 1.5f, 0.0f));
    camera->setRadius(3.0f);

    scene = std::make_shared<Scene>(camera);
    scene->addEnvironmentMap("sunflowers_puresky_4k.hdr");
    scene->addSun(44.0f, 215.0f, glm::vec3(1.0f), 5.0f);

    auto groundMat = std::make_shared<Material>(glm::vec3(0.58f, 0.38f, 0.16f), 0.9f);
    auto groundMesh = std::make_shared<Mesh>();
    groundMesh->addVertex(
        glm::vec3(-5.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f));
    groundMesh->addVertex(
        glm::vec3(5.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f));
    groundMesh->addVertex(
        glm::vec3(5.0f, 0.0f, -5.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f));
    groundMesh->addVertex(
        glm::vec3(-5.0f, 0.0f, -5.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f));
    groundMesh->addIndex(0);
    groundMesh->addIndex(1);
    groundMesh->addIndex(2);
    groundMesh->addIndex(2);
    groundMesh->addIndex(3);
    groundMesh->addIndex(0);
    auto groundNode = std::make_unique<SceneNode>();
    groundNode->addMesh(groundMesh, groundMat);
    scene->addSceneNode(groundNode);

    std::vector<glm::vec3> lightPositions = {
        glm::vec3(0.0f, 0.0f, -0.75f),
        glm::vec3(-0.5f, 0.0f, 1.3f),
        glm::vec3(2.0f, 0.5f, 0.0f)
    };
    std::vector<glm::vec4> lightColors = {
        glm::vec4(1.0f, 1.0f, 1.0f, 3.0f),
        glm::vec4(0.4f, 0.78f, 1.0f, 1.0f),
        glm::vec4(1.0f, 0.63f, 0.3f, 5.0f)
    };
    auto lightsNode = std::make_unique<SceneNode>();
    for(int l=0; l<3; l++) {
        auto sceneNode = std::make_unique<SceneNode>();
        auto light = std::make_unique<Light>(lightPositions[l], -lightPositions[l]);
        light->setRange(3.0f);
        light->setColor(glm::vec3(lightColors[l]));
        light->setIntensity(lightColors[l].w);
        sceneNode->addLight(light);
        lightsNode->addChild(sceneNode);
    }
    scene->addSceneNode(lightsNode);

    auto testSpecies = PlantSpecies();
    testSpecies.addPlant(glm::vec3(0.0f), 0.1f);
    scene->addPlants(testSpecies);

    PlantRenderer renderer(context, scene);

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