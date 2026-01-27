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
    speciesUniforms.maxAge = m_maxAge;
    speciesUniforms.growthSpeed = m_growthSpeed;
    speciesUniforms.maxChildren = m_maxChildren;
    speciesUniforms.minExtent = m_minBranchLength;
    speciesUniforms.maxExtent = m_maxBranchLength;
    speciesUniforms.minRadius = m_minBranchRadius;
    for(int c=0; c<5; c++) {
        speciesUniforms.sizeDecrease[4*c] = m_branchSizeDecrease[c];
        speciesUniforms.branchingThetas[4*c] = m_branchingThetas[c];
        speciesUniforms.branchingPhis[4*c] = m_branchingPhis[c];
    }
    speciesUniforms.gravitropsim = m_gravitropism;
    return speciesUniforms;
}

uint32_t PlantSpecies::getMaxModuleOrder() {
    return m_maxModuleOrder;
}

void PlantSpecies::setIndex(uint32_t index) {
    m_index = index;
}

void PlantSpecies::addPlant(glm::vec3 position, float initialAge, glm::vec3 direction) {
    m_plantPositions.emplace_back(position);
    m_initialDirections.emplace_back(direction);
    m_initialAges.emplace_back(initialAge * m_maxAge);
    m_numPlants++;
}

uint32_t PlantSpecies::getMaxNodesPerPrototype() {
    uint32_t base = 1;
    uint32_t sum = 0;
    for(uint32_t l=0; l<m_maxNodeOrder; l++) {
        sum += base;
        base *= m_maxChildren;
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
        uint32_t maxOffset = 1 + static_cast<uint32_t>((1.0f - lambda) * static_cast<float>(glm::pow(m_maxChildren, m_maxNodeOrder-1) - 1));
        uint32_t offset, numEmpty = 0;
        uint32_t base = 1;
        for(uint32_t l=0; l<m_maxNodeOrder; l++) {
            offset = glm::min(base, maxOffset);
            nodes.resize(nodeOffset + offset);
            for(uint32_t o=0; o<offset; o++) {
                nodes[nodeOffset+o].status = 1;
                nodes[nodeOffset+o].age = m_maxAge * (1.0f - (static_cast<float>(l) / static_cast<float>(m_maxNodeOrder)));
                nodes[nodeOffset+o].order = l;
                if(l > 0) {
                    nodes[nodeOffset+o].parentIndex = nodeOffset - (base / m_maxChildren - numEmpty) + (o / m_maxChildren);
                }
                if(l < m_maxNodeOrder-1) {
                    for(uint32_t c=0; c<m_maxChildren; c++) {
                        if(o*m_maxChildren+c < glm::min(base*m_maxChildren, maxOffset)) {
                            nodes[nodeOffset+o].childIndices[c] = nodeOffset + offset + o * m_maxChildren + c;
                            nodes[nodeOffset+o].numChildren++;
                        }
                    }
                }
            }
            nodeOffset += offset;
            numEmpty = base - offset;
            base *= m_maxChildren;
        }

        numNodes += nodeOffset - firstNode;
        prototypes[firstPrototype+p].numNodes = nodeOffset - firstNode;
        prototypes[firstPrototype+p].firstNode = firstNode;

        uint32_t numBuds = 0;
        for(uint32_t n=0; n<nodeOffset-firstNode; n++) {
            if(nodes[firstNode+n].numChildren == 0 && numBuds < 8) {
                prototypes[firstPrototype+p].budIndices[4*numBuds] = n;
                prototypes[firstPrototype+p].budWeights[4*numBuds] = listIndexToNodeWeight(firstNode+n, nodes, firstPrototype+p, prototypes);
                numBuds++;
            }
        }
        prototypes[firstPrototype+p].numBuds = numBuds;

        firstNode = nodeOffset;
    }

    return m_numPrototypes;
}

float PlantSpecies::listIndexToNodeWeight(uint32_t listIndex, std::vector<Node> &nodes, uint32_t prototypeIndex, std::vector<Prototype> &prototypes) {
    uint32_t rootIndex = prototypes[prototypeIndex].firstNode; //index of the root node of the prototype
    float lambda = prototypes[prototypeIndex].lambda;
    uint32_t currIndex = listIndex;

    uint32_t level = 0;
    uint32_t base = 1;
    uint32_t offset = 0;
    while(currIndex > rootIndex) {
        uint32_t child = 0;
        while(nodes[nodes[currIndex].parentIndex].childIndices[child] != currIndex
            && child < m_maxChildren) {
            child++;
        }
        //if(child >= nodes[nodes[currIndex].parentIndex].numChildren) {
        //    return false;
        //}

        offset += child * base;
        base *= m_maxChildren;
        level++;

        currIndex = nodes[currIndex].parentIndex;
    }

    float weight = 1.0f;
    for(uint32_t l=0; l<level; l++) {
        base /= m_maxChildren;
        uint32_t child = offset / base;
        offset -= child * base;

        //if(child >= nodes[currIndex].numChildren) {
        //    return false;
        //}
        currIndex = nodes[currIndex].childIndices[child];

        if(child == 0) {
            weight *= lambda;
        } else {
            weight *= (1.0f - lambda) / static_cast<float>(m_maxChildren-1);
        }
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
        auto rotAngle = dist(mt) * 2.0f * glm::pi<float>();
        modules[firstModule+p].rotation = glm::vec4(glm::sin(0.5f * rotAngle) * glm::vec3(0.0f, 1.0f, 0.0f), glm::cos(0.5f * rotAngle));
        modules[firstModule+p].status = 1;
        auto initialLength = m_maxBranchLength * (m_initialAges[p] / m_maxAge);
        modules[firstModule+p].center = 0.5f * initialLength * m_initialDirections[p];
        modules[firstModule+p].radius = 0.5f * initialLength;
        modules[firstModule+p].age = m_initialAges[p];
        modules[firstModule+p].vigor = 1.0f;
        modules[firstModule+p].speciesIndex = m_index;
        modules[firstModule+p].prototypeIndex = static_cast<uint32_t>((2.0f * m_lambda - 1.0f) * static_cast<float>(m_numPrototypes - 1));
    }

    return m_numPlants;
}