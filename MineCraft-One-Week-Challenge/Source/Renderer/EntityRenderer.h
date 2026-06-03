#ifndef ENTITYRENDERER_H_INCLUDED
#define ENTITYRENDERER_H_INCLUDED

#include <vector>
#include <string>
#include <unordered_set>
#include <glad/glad.h>
#include <unordered_map>
#include "../Maths/glm.h"
#include "../Shaders/BasicShader.h"
#include "../Texture/BasicTexture.h"
#include "../Model.h"
#include "../Util/tiny_gltf.h"

class Camera;

struct GltfMeshPart {
    Model* model = nullptr;
    int nodeIndex = -1;
};

struct EntityRenderData {
    glm::vec3 position;
    glm::vec3 rotation;
    int state = 0;          // 0=Patrol, 1=Chase, 2=Attack, 3=Hurt, 4=Dead
    float animTimer = 0.0f;
    float deathAnimTimer = 0.0f;
    float attackCooldown = 0.0f;
    float scale = 0.6f;
    float rotationYOffset = 180.0f;
    bool isHurt = false;
};

class EntityRenderer {
public:
    EntityRenderer(const std::string& modelPath, const std::string& textureName);
    ~EntityRenderer();

    void addEntity(const EntityRenderData& e);
    void render(const Camera& camera);

private:
    bool loadGltf(const char* path);
    void buildMeshParts(tinygltf::Model& gltf);
    void buildParentMap(tinygltf::Model& gltf);
    glm::mat4 getNodeWorldTransform(int nodeIdx, float animTime);

    void identifyJointNodes();

    std::vector<GltfMeshPart> m_meshParts;
    std::vector<int> m_parentMap;
    std::unordered_set<int> m_jointNodes; // leg-joint node indices for procedural animation
    tinygltf::Model m_gltfModel;
    BasicTexture m_texture;
    BasicShader m_shader;
    std::vector<EntityRenderData> m_entities;

    GLuint m_tintLocation = 0;
};

#endif
