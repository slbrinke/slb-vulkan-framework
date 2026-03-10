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

int screenWidth = 1300;
int screenHeight = 700;

std::shared_ptr<Context> context = nullptr;
std::shared_ptr<Camera> camera = nullptr;
std::shared_ptr<Scene> scene = nullptr;

int main() {
    context = std::make_shared<Context>(screenWidth, screenHeight, "Vulkan Framework");
    glfwSetKeyCallback(context->getWindow().get(), keyCallback);

    camera = std::make_shared<Camera>(screenWidth, screenHeight, context->getWindow());
    camera->setPosition(glm::vec3(0.0f, 0.1f, 0.0f));

    scene = std::make_shared<Scene>(camera);
    scene->addEnvironmentMap("sunflowers_puresky_4k.hdr", 180.0f);
    //scene->addSun(30.0f, 50.0f, glm::vec3(0.85f, 0.67f, 0.29f), 1.0f);
    scene->addSun(44.0f, 35.0f, glm::vec3(1.0f), 3.0f);

    auto groundMat = std::make_shared<Material>(glm::vec3(0.58f, 0.38f, 0.16f), 0.9f);
    groundMat->setDiffuseTexture("brown_mud_dry_diff_4k.jpg");
    groundMat->setNormalTexture("brown_mud_dry_nor_gl_4k.jpg");
    auto groundMesh = std::make_shared<Mesh>();
    float groundScale = 1.0f;
    groundMesh->addVertex(
        glm::vec3(-groundScale, 0.0f, groundScale),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f));
    groundMesh->addVertex(
        glm::vec3(groundScale, 0.0f, groundScale),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(groundScale, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f));
    groundMesh->addVertex(
        glm::vec3(groundScale, 0.0f, -groundScale),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(groundScale, groundScale),
        glm::vec3(1.0f, 0.0f, 0.0f));
    groundMesh->addVertex(
        glm::vec3(-groundScale, 0.0f, -groundScale),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.0f, groundScale),
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

    auto modelNode = std::make_unique<SceneNode>();
    auto compostBagsNode = std::make_unique<SceneNode>();
    ResourceLoader::loadModel("compost_bags_4k", compostBagsNode);
    compostBagsNode->scale(0.5f);
    modelNode->addChild(compostBagsNode);
    ResourceLoader::loadModel("watering_can_metal_01_4k", modelNode);
    modelNode->getChildren().back()->setPosition(glm::vec3(0.1f, 0.0f, 0.3f));
    modelNode->getChildren().back()->rotate(315.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    ResourceLoader::loadModel("trowel_01_4k", modelNode);
    modelNode->getChildren().back()->setPosition(glm::vec3(-0.1f, 0.03f, 0.35f));
    modelNode->getChildren().back()->rotate(23.0f, glm::vec3(-1.0f, 0.0f, 0.0f));
    modelNode->getChildren().back()->rotate(50.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    scene->addSceneNode(modelNode);

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

    DeferredRenderer renderer(context, scene);

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