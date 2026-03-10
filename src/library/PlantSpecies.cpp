#include "PlantSpecies.h"

PlantSpecies::PlantSpecies() {
    createPrototypes();
}

bool PlantSpecies::hasIndex() {
    return m_index != std::numeric_limits<uint32_t>::max();
}

uint32_t PlantSpecies::getIndex() {
    return m_index;
}

SpeciesUniforms PlantSpecies::getUniformData() {
    SpeciesUniforms speciesUniforms{};
    speciesUniforms.numPrototypes = m_numPrototypes;
    speciesUniforms.maxModuleOrder = m_maxModuleOrder;
    speciesUniforms.maxNodeChildren = m_maxChildNodes;
    speciesUniforms.minExtent = m_minBranchLength;
    speciesUniforms.maxExtent = m_maxBranchLength;
    speciesUniforms.minRadius = m_minBranchRadius;
    speciesUniforms.maxRadius = m_minBranchRadius; //TO DO: find a value that makes sense
    for(int c=0; c<5; c++) {
        speciesUniforms.sizeDecrease[4*c] = m_branchSizeDecrease[c];
        speciesUniforms.branchingThetas[4*c] = m_branchingThetas[c];
        speciesUniforms.branchingPhis[4*c] = m_branchingPhis[c];
    }
    speciesUniforms.gravitropism = m_gravitropism;
    speciesUniforms.growthSpeed = m_growthSpeed;
    speciesUniforms.maxModuleAge = m_maxModuleAge;
    speciesUniforms.maxNodeAge = m_maxModuleAge / static_cast<float>(m_maxModuleOrder+1);
    speciesUniforms.maturityOrder = m_maturityOrder;
    speciesUniforms.initialLambda = m_initialLambda;
    speciesUniforms.matureLambda = m_matureLambda;
    speciesUniforms.minVigor = m_minVigor;
    speciesUniforms.maxRotChanges = m_maxRotChanges;
    speciesUniforms.rotChangeAngle = glm::radians(m_rotChangeAngle);
    return speciesUniforms;
}

uint32_t PlantSpecies::getMaxModuleOrder() {
    return m_maxModuleOrder;
}

void PlantSpecies::setIndex(uint32_t index) {
    m_index = index;
}

void PlantSpecies::setBranchingAngles(uint32_t childIndex, float theta, float phi) {
    if(childIndex >= m_maxChildNodes) {
        throw std::runtime_error("PLANT SPECIES ERROR: Each branch segment can only have " + std::to_string(m_maxChildNodes) + " child segments.");
    }
    m_branchingThetas[childIndex] = glm::radians(theta);
    m_branchingPhis[childIndex] = glm::radians(phi);
}

void PlantSpecies::setInitialLambda(float lambda) {
    m_initialLambda = lambda;
}

void PlantSpecies::setMatureLambda(float lambda) {
    m_matureLambda = lambda;
}

void PlantSpecies::setMaturityOrder(uint32_t order) {
    m_maturityOrder = order;
}

void PlantSpecies::setNumOptimizationIterations(uint32_t numIterations) {
    m_maxRotChanges = numIterations;
}

void PlantSpecies::addPlant(glm::vec3 position, float initialAge, glm::vec3 direction) {
    m_plantPositions.emplace_back(position);
    m_initialDirections.emplace_back(direction);
    m_initialAges.emplace_back(initialAge * m_maxModuleAge);
    m_numPlants++;
}

uint32_t PlantSpecies::getMaxNodesPerPrototype() {
    uint32_t base = 1;
    uint32_t sum = 0;
    for(uint32_t l=0; l<=m_maxNodeOrder; l++) {
        sum += base;
        base *= m_maxChildNodes;
    }
    return sum;
}

void PlantSpecies::createPrototypes() {
    uint32_t numNodes = 0;
    m_prototypes.resize(m_numPrototypes);
    for(uint32_t p=0; p<m_numPrototypes; p++) {
        //resource distribution lambda varies from 0.5 to 1.0
        float lambda = static_cast<float>(p) / glm::max(1.0f, static_cast<float>(m_numPrototypes-1));
        m_prototypes[p].lambda = 0.5 + 0.5 * lambda;
        m_prototypes[p].firstNode = numNodes;

        //add nodes level per level:
        //the greater lambda the more possible leaf nodes
        uint32_t maxPerLevel = 1 + static_cast<uint32_t>((1.0f - lambda) * static_cast<float>(glm::pow(m_maxChildNodes, m_maxNodeOrder) - 1));
        uint32_t nodesPerLevel, numEmpty = 0;
        uint32_t base = 1;
        for(uint32_t l=0; l<=m_maxNodeOrder; l++) {
            nodesPerLevel = glm::min(base, maxPerLevel);
            m_nodes.resize(numNodes + nodesPerLevel);
            for(uint32_t n=0; n<nodesPerLevel; n++) {
                m_nodes[numNodes+n].status = 1;
                m_nodes[numNodes+n].age = m_maxModuleAge * (1.0f - (static_cast<float>(l) / static_cast<float>(m_maxNodeOrder+1)));
                m_nodes[numNodes+n].order = l;
                if(l > 0) {
                    m_nodes[numNodes+n].parentIndex = numNodes - (base / m_maxChildNodes - numEmpty) + (n / m_maxChildNodes);
                }
                if(l < m_maxNodeOrder) {
                    for(uint32_t c=0; c<m_maxChildNodes; c++) {
                        if(n*m_maxChildNodes+c < glm::min(base*m_maxChildNodes, maxPerLevel)) {
                            m_nodes[numNodes+n].childIndices[c] = numNodes + nodesPerLevel + n * m_maxChildNodes + c;
                            m_nodes[numNodes+n].numChildren++;
                        }
                    }
                }
            }
            numNodes += nodesPerLevel;
            numEmpty = base - nodesPerLevel;
            base *= m_maxChildNodes;
        }

        m_prototypes[p].numNodes = numNodes - m_prototypes[p].firstNode;

        //count number of buds and calculate their relative weights
        uint32_t numBuds = 0;
        for(uint32_t n=0; n<m_prototypes[p].numNodes; n++) {
            if(m_nodes[m_prototypes[p].firstNode+n].numChildren == 0 && numBuds < 8) {
                m_prototypes[p].budIndices[4*numBuds] = n;
                m_prototypes[p].initialBudWeights[4*numBuds] = listIndexToNodeWeight(m_initialLambda, m_prototypes[p].firstNode+n, p);
                m_prototypes[p].matureBudWeights[4*numBuds] = listIndexToNodeWeight(m_matureLambda, m_prototypes[p].firstNode+n, p);
                numBuds++;
            }
        }
        m_prototypes[p].numBuds = numBuds;
    }
}

void PlantSpecies::recordPrototypes(uint32_t &numPrototypes, std::vector<Prototype> &prototypes, uint32_t &numNodes, std::vector<Node> &nodes) {
    m_prototypeOffset = numPrototypes;
    m_nodeOffset = numNodes;

    prototypes.resize(m_prototypeOffset + m_numPrototypes);
    for(uint32_t p=0; p<m_numPrototypes; p++) {
        prototypes[m_prototypeOffset+p].lambda = m_prototypes[p].lambda;
        prototypes[m_prototypeOffset+p].numNodes = m_prototypes[p].numNodes;
        prototypes[m_prototypeOffset+p].firstNode = m_nodeOffset + m_prototypes[p].firstNode;
        prototypes[m_prototypeOffset+p].numBuds = m_prototypes[p].numBuds;
        for(uint32_t b=0; b<m_prototypes[p].numBuds; b++) {
            prototypes[m_prototypeOffset+p].budIndices[4*b] = m_prototypes[p].budIndices[4*b];
            prototypes[m_prototypeOffset+p].initialBudWeights[4*b] = m_prototypes[p].initialBudWeights[4*b];
            prototypes[m_prototypeOffset+p].matureBudWeights[4*b] = m_prototypes[p].matureBudWeights[4*b];
        }
    }

    uint32_t addedNodes = static_cast<uint32_t>(m_nodes.size());
    nodes.resize(m_nodeOffset + addedNodes);
    for(uint32_t n=0; n<addedNodes; n++) {
        nodes[m_nodeOffset+n].status = 1;
        nodes[m_nodeOffset+n].age = m_nodes[n].age;
        nodes[m_nodeOffset+n].order = m_nodes[n].order;
        nodes[m_nodeOffset+n].parentIndex = m_nodeOffset + m_nodes[n].parentIndex;
        nodes[m_nodeOffset+n].numChildren = m_nodes[n].numChildren;
        for(uint32_t c=0; c<m_nodes[n].numChildren; c++) {
            nodes[m_nodeOffset+n].childIndices[c] = m_nodeOffset + m_nodes[n].childIndices[c];
        }
    }

    numPrototypes += m_numPrototypes;
    numNodes += addedNodes;
}

float PlantSpecies::listIndexToNodeWeight(float lambda, uint32_t listIndex, uint32_t prototypeIndex) {
    uint32_t rootIndex = m_prototypes[prototypeIndex].firstNode; //index of the root node of the prototype
    uint32_t currIndex = listIndex;

    uint32_t level = 0;
    uint32_t base = 1;
    uint32_t offset = 0;
    while(currIndex > rootIndex) {
        uint32_t child = 0;
        while(m_nodes[m_nodes[currIndex].parentIndex].childIndices[child] != currIndex
            && child < m_maxChildNodes) {
            child++;
        }
        //if(child >= nodes[nodes[currIndex].parentIndex].numChildren) {
        //    return false;
        //}

        offset += child * base;
        base *= m_maxChildNodes;
        level++;

        currIndex = m_nodes[currIndex].parentIndex;
    }

    float weight = 1.0f;
    for(uint32_t l=0; l<level; l++) {
        base /= m_maxChildNodes;
        uint32_t child = offset / base;
        offset -= child * base;

        float total = lambda;
        for(uint32_t c=1; c<m_nodes[currIndex].numChildren; c++) {
            total += (1.0f - lambda) / static_cast<float>(m_maxChildNodes-1);
        }
        if(child == 0) {
            weight *= lambda / total;
        } else {
            weight *= ((1.0f - lambda) / static_cast<float>(m_maxChildNodes-1)) / total;
        }

        //if(child >= nodes[currIndex].numChildren) {
        //    return false;
        //}
        currIndex = m_nodes[currIndex].childIndices[child];
    }

    return weight;
}

uint32_t PlantSpecies::createModules(std::vector<Module> &modules) {
    if(!hasIndex()) {
        throw std::runtime_error("PLANT SPECIES ERROR: Plant modules cannot be initialized before the species index is assigned.");
    }

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    uint32_t firstModule = modules.size();
    modules.resize(firstModule + m_numPlants);
    for(uint32_t p=0; p<m_numPlants; p++) {
        modules[firstModule+p].position = m_plantPositions[p];
        modules[firstModule+p].status = 1;
        auto rotAngle = dist(mt) * 2.0f * glm::pi<float>();
        modules[firstModule+p].rotation = glm::eulerAngles(glm::quat(glm::cos(0.5f*rotAngle), glm::sin(0.5f*rotAngle) * m_initialDirections[p]));
        modules[firstModule+p].age = m_initialAges[p];
        auto initialLength = m_maxBranchLength * (m_initialAges[p] / m_maxModuleAge);
        modules[firstModule+p].center = 0.5f * initialLength * m_initialDirections[p];
        modules[firstModule+p].radius = 0.5f * initialLength;
        modules[firstModule+p].vigor = 1.0f;
        modules[firstModule+p].speciesIndex = m_index;
        modules[firstModule+p].prototypeIndex = m_prototypeOffset + glm::min(static_cast<uint32_t>((2.0f * m_initialLambda - 1.0f) * static_cast<float>(m_numPrototypes)), m_numPrototypes-1);
    }

    return m_numPlants;
}

void PlantSpecies::addModuleToMesh(Module &module, std::shared_ptr<Mesh> &mesh) {
    //TO DO!!!
    /*
    auto &prototype = m_prototypes[module.prototypeIndex - m_prototypeOffset];
    for(uint32_t n=0; n<prototype.numNodes; n++) {
        glm::mat4 nodeModel, parentModel;
        if(listIndexToNodeModel(prototype.firstNode+n, module, nodeModel, parentModel)) {
            int resolution = 5;
            float step = 1.0f / static_cast<float>(resolution);
            for(int i=0; i<resolution; i++) {
                float hRel = static_cast<float>(i) * step;
                float phi = hRel * 2.0f * glm::pi<float>();
                mesh->addVertex(
                    glm::vec3(parentModel * glm::vec4(m_minBranchRadius * glm::cos(phi), 0.0f, -m_minBranchRadius * glm::sin(phi), 1.0f)),
                    glm::vec3(0.0f),
                    glm::vec2(hRel, 0.0f),
                    glm::vec3(0.0f)
                );
                mesh->addVertex(
                    glm::vec3(nodeModel * glm::vec4(m_minBranchRadius * glm::cos(phi), 0.0f, -m_minBranchRadius * glm::sin(phi), 1.0f)),
                    glm::vec3(0.0f),
                    glm::vec2(hRel, 1.0f),
                    glm::vec3(0.0f)
                );
            }
        }
    }
        */
}