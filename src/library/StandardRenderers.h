#ifndef SLBVULKAN_STANDARDRENDERERS_H
#define SLBVULKAN_STANDARDRENDERERS_H

#include "Renderer.h"

/**
 * Simple forward renderer using physically-based shading.
 * 
 * Iterates over all light sources in the main fragment shader to accumulate all lighting.
 */
class ForwardRenderer : public Renderer {
public:
    ForwardRenderer(std::shared_ptr<Context> &context, std::shared_ptr<Camera> &camera, std::shared_ptr<Scene> &scene);
    ~ForwardRenderer() = default;

private:
    void setUpRenderOutput() override;
    void setUpRenderSteps() override;
};

/**
 * Deferred renderer using physically-based shading.
 * 
 * Records geometry data in a gbuffer and then renders light proxy geometry in a second pass.
 */
class DeferredRenderer : public Renderer {
public:
    DeferredRenderer(std::shared_ptr<Context> &context, std::shared_ptr<Camera> &camera, std::shared_ptr<Scene> &scene);
    ~DeferredRenderer() = default;

private:
    void setUpRenderOutput() override;
    void setUpRenderSteps() override;
    
};

#endif //SLBVULKAN_STANDARDRENDERERS_H