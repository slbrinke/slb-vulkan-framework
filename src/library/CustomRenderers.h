#ifndef SLBVULKAN_CUSTOMRENDERERS_H
#define SLBVULKAN_CUSTOMRENDERERS_H

#include "Renderer.h"
#include "PlantSpecies.h"

/**
 * Specialized renderer for plant generation.
 * 
 * Visualizes scene geometry in a simplified way without shadows.
 * Simulates plant development based on the plant modules provided in the scene.
 * The simulation is based on:
 * Makowski et al. "Synthetic Silviculture: Multi-scale Modeling of Plant Ecosystems"
 */
class PlantRenderer : public Renderer {
public:
    PlantRenderer(std::shared_ptr<Context> &context, std::shared_ptr<Scene> &scene);
    ~PlantRenderer() = default;

    void constructModuleGeometry();

private:

    void setUpRenderOutput() override;
    void setUpRenderSteps() override;

};

#endif //SLBVULKAN_CUSTOMRENDERERS_H