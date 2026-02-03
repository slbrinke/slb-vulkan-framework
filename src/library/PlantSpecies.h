#ifndef SLBVULKAN_PLANTSPECIES_H
#define SLBVULKAN_PLANTSPECIES_H

#include <vector>
#include <random>
#include <stdexcept>
#include <iostream>

#include <glm/ext.hpp>

/**
 * GPU representation of a plant species.
 * 
 * Contains all relevant parameters used for plant simulation and rendering.
 * For each plant species an instance is added to the plant species uniform buffer.
 */
struct SpeciesUniforms {
    uint32_t numPrototypes; /**< Total number of module prototypes */
    uint32_t firstPrototype; /**< First index of the module prototypes within the prototypes uniform buffer */
    uint32_t maxModuleOrder; /**< Maximum order a module can have in the hierarchy */
    uint32_t maxNodeChildren; /**< Maximum number of child branches sprouting out from a branch segment */
    float minExtent; /**< Minimum length of a branch segment */
    float maxExtent; /**< Maximum length of a branch segment */
    float minRadius; /**< Minimum radius of a branch segment */
    float maxRadius; /**< Maximum radius of a branch segment */
    float sizeDecrease[20]; /**< Length of each child branch segment in relation to the parent branch segment length */
    float branchingThetas[20]; /**< Vertical branching angle of each child branch segment relative to the orientation of the parent */
    float branchingPhis[20]; /**< Horizontal branching angle of each child branch segment relative to the orientation of the parent */
    float gravitropism; /**< Strength of the impact of gravity on branch segments */
    float growthSpeed; /**< Growth rate in world scale per simulated second */
    float maxModuleAge; /**< Maximum age at which a module stops growing */
    float maxNodeAge; /**< Maximum age at which a branch segment stops growing */
    float minVigor; /**< Minimum vigor required for a module to stay alive */
    uint32_t maxRotChanges; /**< Maximum number of rotation changes per orientation optimization */
    float rotChangeAngle; /**< Small angle by which the rotation of a module is changed in one iteration */
    float pad;
};

/**
 * Bigger scale building block making up a plant.
 * 
 * Utilizes plant self-similarity to encompass a cluster of multiple branches into one element.
 * Contains an overall transform and branch topography (just connections).
 * But the exact geometry of the included branch segments is determined by species parameters.
 * 
 * The status paremeter is set to:
 * 0 if it is not initialized or dead,
 * 1 if it is growing,
 * 2 if it is fully grown,
 * 3 if it has generated child modules.
 */
struct Module {
    glm::vec3 position; /**< Origin of the module in world coordinates */
    uint32_t status; /**< State of the module within its lifetime */
    //glm::vec4 rotation; /**< Rotation of the module in world coordinates */
    glm::vec3 rotation; /**< Rotation of the module as euler angles */
    float age; /**< Time passed since module creation in simulated seconds */
    glm::vec3 center; /**< Center of all branch and bud positions in the local coordinates of the module */
    float radius; /**< Radius of the sphere encompassing the module at its current size */
    float maxFlux; /**< Maximum approximated flux received by any child module down the hierarchy */
    float vigor; /**< Resource value characterizing the current state of the module */
    uint32_t numRotChanges; /**< Number of rotation changes in the current orientation optimization */
    uint32_t order = 0; /**< Depth within the module hierarchy */
    uint32_t speciesIndex; /**< Index of the plant species within the uniform buffer */
    uint32_t prototypeIndex; /**< Index of the module prototype within the uniform buffer */
    uint32_t parentIndex = 0; /**< Index of the parent module within the module buffer */
    uint32_t numChildren = 0; /**< Number of child modules originating from this module */
    uint32_t childIndices[8]; /**< Indices of the child modules within the module buffer */
};

/**
 * Module templates dictating the basic shapes different modules can take.
 * 
 * The branching structure of a module is represented by a hierarchy of branch segments.
 * Node structs are used to represent branch segments.
 */
struct Prototype {
    float lambda; /**< Amount of growth directed along the main axis relative to branching axes */
    //float determinacy; /**< Temporarily not in use!!! */
    uint32_t numNodes; /**< Total number of nodes */
    uint32_t firstNode; /**< First index of the nodes representing branch segments in the node buffer */
    uint32_t numBuds; /**< Number of terminal nodes (buds) child modules can be attached to */
    uint32_t budIndices[32]; /**< Indices of the terminal nodes in the node buffer */
    float budWeights[32]; /**< Lambda values of the terminal nodes accumulated through the prototype hierarchy */
};

/**
 * General connecting element.
 * 
 * Can be used for different purposes based on the implementation.
 */
struct Node {
    uint32_t status;
    float age;
    uint32_t ref1;
    uint32_t ref2;
    uint32_t order = 0;
    uint32_t parentIndex = 0;
    uint32_t numChildren = 0;
    uint32_t childIndices[5];
};

/**
 * Species characterizing a type of plant.
 * 
 * Manages all plants of this species within the scene.
 * Module and prototype data can be added to a scene and simulated as well as visualized by a PlantRenderer.
 */
class PlantSpecies {
public:
    PlantSpecies() = default;
    ~PlantSpecies() = default;

    /**
     * Check whether an index has been assigned.
     * 
     * If this is not the case the species and the included plants have not been added to a scene yet.
     * 
     * @return true if the plant species has a valid index
     */
    bool hasIndex();

    /**
     * Return the index assigned to the plant species by the scene.
     * 
     * This works off the assumption that there is only one relevant scene.
     * 
     * @return index identifying the plant species in the scene
     */
    uint32_t getIndex();

    /**
     * Provide plant data to be added to a uniform buffer.
     * 
     * @return SpeciesUniforms instance containing all relevant plant properties
     */
    SpeciesUniforms getUniformData();

    uint32_t getMaxModuleOrder();

    /**
     * Assign an index to the plant species.
     * 
     * During simulation and rendering the index can be used to specify the species of a plant module.
     * 
     * @param index unique index identifying the plant species
     */
    void setIndex(uint32_t index);

    /**
     * Add a plant of this type.
     * 
     * The plant is initialized with a single module.
     * Per default the first branch segment initially points upwards.
     * The initial age of the module at the beginning of the simulation is determined relative to the maximum module age.
     * 
     * @param position location the plant sprouts from
     * @param initialAge relative initial module age between 0.0 and 1.0
     * @param direction orientation of the initial module
     */
    void addPlant(glm::vec3 position, float initialAge = 0.0f, glm::vec3 direction = glm::vec3(0.0f, 1.0f, 0.0f));

    /**
     * Return the maximum number of nodes contained in a prototype of the species.
     * 
     * The prototypes can have a different number of nodes, but the maximum is used as an upper limit.
     */
    uint32_t getMaxNodesPerPrototype();

    /**
     * Set up the module prototypes for this plant species.
     * 
     * @param prototypes overall list the prototype data of this species is added to
     * @param numNodes total node counter the number of branch segment nodes is added to
     * @param nodes overall list the nodes representing branch segments is added to
     * @return number of added prototypes
     */
    uint32_t createPrototypes(std::vector<Prototype> &prototypes, uint32_t &numNodes, std::vector<Node> &nodes);

    /**
     * Set up the initial modules for the defined plants.
     * 
     * The species index must be assigned before this point or the modules cannot be interpreted.
     * No more plants can be added after this is called.
     * 
     * @param modules overall list the modules are added to
     * @return number of added modules
     */
    uint32_t createModules(std::vector<Module> &modules);

private:
    /**
     * Accumulate branch weights within a module prototype.
     * 
     * The node structure of a prototype is iterated to multiply the lambda values.
     * 
     * @param listIndex index of a branch segment within the node buffer
     * @param nodes list of all nodes comprising the module prototypes
     * @param prototypeIndex index of a prototype within the prototype buffer
     * @param prototypes list of all prototypes modules can be assigned
     * @return amount of resources directed towards a node relative to the module prototype
     */
    float listIndexToNodeWeight(uint32_t listIndex, std::vector<Node> &nodes, uint32_t prototypeIndex, std::vector<Prototype> &prototypes);

    uint32_t m_index = std::numeric_limits<uint32_t>::max(); /**< Unique index identifying the plant species in the scene */

    uint32_t m_numPlants = 0; /**< Number of plants of this species */
    std::vector<glm::vec3> m_plantPositions; /**< Seeding locations of the plants */
    std::vector<glm::vec3> m_initialDirections; /**< Orientation of the initial branch segments of the plants */
    std::vector<float> m_initialAges; /**< Age of the initial modules of the plants at the beginning of the simulation */

    float m_maxAge = 3.0f; /**< Maximum age at which a module stops growing */
    float m_growthSpeed = 0.3f; /**< Growth rate in world scale per simulated second */
    float m_lambda = 0.5f; /**< Amount of growth directed along the main axis relative to branching axes */
    uint32_t m_maxModuleOrder = 5; /**< Maximum depth of the plant hierarchy in number of modules */
    float m_minVigor = 0.15f; /**< Minimum required vigor value for a module */
    uint32_t m_maxRotChanges = 3; /**< Maximum number of iterations per module orientation optimization */
    float m_rotChangeAngle = 0.05f; /**< Angle of each iteration during module orientation optimization */

    uint32_t m_numPrototypes = 3; /**< Number of prototypes defined for this species */
    uint32_t m_maxNodeOrder = 3; /**< Maximum branching depth within a module */
    uint32_t m_maxChildren = 2; /**< Maximum number of child branches sprouting out from a branch segment */

    float m_minBranchLength = 0.01f; /**< Minimum length of a branch segment */
    float m_maxBranchLength = 0.3f; /**< Maximum length of a branch segment */
    float m_minBranchRadius = 0.01f; /**< Minimum radius of a branch segment */
    float m_branchSizeDecrease[5] = {0.9f, 0.9f, 0.0f, 0.0f, 0.0f}; /**< Length of each child branch segment in relation to the parent branch segment length */
    float m_branchingThetas[5] = {glm::radians(15.0f), glm::radians(15.0f), 0.0f, 0.0f, 0.0f}; /**< Vertical branching angle of each child branch segment relative to the orientation of the parent */
    float m_branchingPhis[5] = {glm::radians(0.0f), glm::radians(180.0f), 0.0f, 0.0f, 0.0f}; /**< Horizontal branching angle of each child branch segment relative to the orientation of the parent */
    float m_gravitropism = 0.01f; /**< Strength of the impact of gravity on branch segments */

};

#endif //SLBVULKAN_PLANTSPECIES_H