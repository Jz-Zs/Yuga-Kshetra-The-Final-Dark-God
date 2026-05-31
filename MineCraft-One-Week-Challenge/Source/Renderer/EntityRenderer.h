#ifndef ENTITYRENDERER_H_INCLUDED
#define ENTITYRENDERER_H_INCLUDED

#include <vector>
#include <glad/glad.h>
#include <unordered_map>
#include "../Maths/glm.h"
#include "../Shaders/BasicShader.h"
#include "../Texture/BasicTexture.h"
#include "../Entity/PigmanEntity.h"
#include "../Model.h"

// Include tinygltf for type definitions only (no implementation here)
#include "../Util/tiny_gltf.h"

class Camera;

struct GltfMeshPart {
    Model* model = nullptr;
    int nodeIndex = -1;
};

class EntityRenderer {
public:
    EntityRenderer();
    ~EntityRenderer();

    void addEntity(const PigmanEntity& e);
    void render(const Camera& camera);

private:
    bool loadGltf(const char* path);
    void buildMeshParts(tinygltf::Model& gltf);
    void buildParentMap(tinygltf::Model& gltf);
    glm::mat4 getNodeWorldTransform(int nodeIdx, float animTime);

    std::vector<GltfMeshPart> m_meshParts;
    std::vector<int> m_parentMap; // nodeIdx → parent node index, -1 = root
    tinygltf::Model m_gltfModel;
    BasicTexture m_texture;
    BasicShader m_shader;
    std::vector<const PigmanEntity*> m_entities;

    GLuint m_tintLocation = 0;
};

#endif
