#include "PlantSpecies.h"

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

uint32_t PlantSpecies::createPrototypes(std::vector<Prototype> &prototypes, uint32_t &numNodes, std::vector<Node> &nodes) {
    uint32_t firstPrototype = prototypes.size();
    uint32_t firstNode = nodes.size();

    prototypes.resize(firstPrototype + m_numPrototypes);
    for(uint32_t p=0; p<m_numPrototypes; p++) {
        float lambda = static_cast<float>(p) / glm::max(1.0f, static_cast<float>(m_numPrototypes-1));
        prototypes[firstPrototype+p].lambda = 0.5 + 0.5 * lambda;
        //prototypes[firstPrototype+p].determinacy = 0.0f;
        
        uint32_t nodeOffset = firstNode;
        uint32_t maxOffset = 1 + static_cast<uint32_t>((1.0f - lambda) * static_cast<float>(glm::pow(m_maxChildNodes, m_maxNodeOrder) - 1));
        uint32_t offset, numEmpty = 0;
        uint32_t base = 1;
        for(uint32_t l=0; l<=m_maxNodeOrder; l++) {
            offset = glm::min(base, maxOffset);
            nodes.resize(nodeOffset + offset);
            for(uint32_t o=0; o<offset; o++) {
                nodes[nodeOffset+o].status = 1;
                nodes[nodeOffset+o].age = m_maxModuleAge * (1.0f - (static_cast<float>(l) / static_cast<float>(m_maxNodeOrder+1)));
                nodes[nodeOffset+o].order = l;
                if(l > 0) {
                    nodes[nodeOffset+o].parentIndex = nodeOffset - (base / m_maxChildNodes - numEmpty) + (o / m_maxChildNodes);
                }
                if(l < m_maxNodeOrder) {
                    for(uint32_t c=0; c<m_maxChildNodes; c++) {
                        if(o*m_maxChildNodes+c < glm::min(base*m_maxChildNodes, maxOffset)) {
                            nodes[nodeOffset+o].childIndices[c] = nodeOffset + offset + o * m_maxChildNodes + c;
                            nodes[nodeOffset+o].numChildren++;
                        }
                    }
                }
            }
            nodeOffset += offset;
            numEmpty = base - offset;
            base *= m_maxChildNodes;
        }

        numNodes += nodeOffset - firstNode;
        prototypes[firstPrototype+p].numNodes = nodeOffset - firstNode;
        prototypes[firstPrototype+p].firstNode = firstNode;

        uint32_t numBuds = 0;
        for(uint32_t n=0; n<nodeOffset-firstNode; n++) {
            if(nodes[firstNode+n].numChildren == 0 && numBuds < 8) {
                prototypes[firstPrototype+p].budIndices[4*numBuds] = n;
                prototypes[firstPrototype+p].initialBudWeights[4*numBuds] = listIndexToNodeWeight(m_initialLambda, firstNode+n, nodes, firstPrototype+p, prototypes);
                prototypes[firstPrototype+p].matureBudWeights[4*numBuds] = listIndexToNodeWeight(m_matureLambda, firstNode+n, nodes, firstPrototype+p, prototypes);
                numBuds++;
            }
        }
        prototypes[firstPrototype+p].numBuds = numBuds;

        firstNode = nodeOffset;
    }

    return m_numPrototypes;
}

float PlantSpecies::listIndexToNodeWeight(float lambda, uint32_t listIndex, std::vector<Node> &nodes, uint32_t prototypeIndex, std::vector<Prototype> &prototypes) {
    uint32_t rootIndex = prototypes[prototypeIndex].firstNode; //index of the root node of the prototype
    uint32_t currIndex = listIndex;

    uint32_t level = 0;
    uint32_t base = 1;
    uint32_t offset = 0;
    while(currIndex > rootIndex) {
        uint32_t child = 0;
        while(nodes[nodes[currIndex].parentIndex].childIndices[child] != currIndex
            && child < m_maxChildNodes) {
            child++;
        }
        //if(child >= nodes[nodes[currIndex].parentIndex].numChildren) {
        //    return false;
        //}

        offset += child * base;
        base *= m_maxChildNodes;
        level++;

        currIndex = nodes[currIndex].parentIndex;
    }

    float weight = 1.0f;
    for(uint32_t l=0; l<level; l++) {
        base /= m_maxChildNodes;
        uint32_t child = offset / base;
        offset -= child * base;

        float total = lambda;
        for(uint32_t c=1; c<nodes[currIndex].numChildren; c++) {
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
        currIndex = nodes[currIndex].childIndices[child];
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
        modules[firstModule+p].prototypeIndex = glm::min(static_cast<uint32_t>((2.0f * m_initialLambda - 1.0f) * static_cast<float>(m_numPrototypes)), m_numPrototypes-1);
    }

    return m_numPlants;
}