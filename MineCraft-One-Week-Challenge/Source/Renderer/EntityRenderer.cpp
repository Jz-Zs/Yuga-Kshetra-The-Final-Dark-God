// Generate tinygltf implementation in this translation unit only
// Must be defined BEFORE EntityRenderer.h includes tiny_gltf.h
#define TINYGLTF_USE_CPP14
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "EntityRenderer.h"
#include "../Camera.h"
#include <cstdint>
#include <iostream>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

EntityRenderer::EntityRenderer()
    : m_shader("Basic", "Basic")
{
    if (!loadGltf("Res/Models/Zoglin/minecraft_-zoglin/scene.gltf")) {
        std::cerr << "EntityRenderer: failed to load Zoglin model\n";
    }
    m_texture.loadFromFile("zoglin");

    // Cache the tintColor uniform location
    m_shader.useProgram();
    m_tintLocation = glGetUniformLocation(m_shader.getID(), "tintColor");
}

EntityRenderer::~EntityRenderer()
{
    for (auto& part : m_meshParts)
        delete part.model;
}

bool EntityRenderer::loadGltf(const char* path)
{
    tinygltf::TinyGLTF loader;
    // Provide no-op image loader — we load textures manually via BasicTexture
    loader.SetImageLoader(
        [](tinygltf::Image*, const int, std::string*, std::string*,
           int, int, const unsigned char*, int, void*) { return true; },
        nullptr);
    std::string err, warn;
    bool ok = loader.LoadASCIIFromFile(&m_gltfModel, &err, &warn, path);
    if (!warn.empty()) std::cerr << "glTF warn: " << warn << "\n";
    if (!err.empty()) std::cerr << "glTF err: " << err << "\n";
    if (!ok) return false;

    buildParentMap(m_gltfModel);
    buildMeshParts(m_gltfModel);
    return !m_meshParts.empty();
}

void EntityRenderer::buildParentMap(tinygltf::Model& gltf)
{
    m_parentMap.assign(gltf.nodes.size(), -1);
    for (size_t i = 0; i < gltf.nodes.size(); i++) {
        for (int child : gltf.nodes[i].children) {
            m_parentMap[child] = (int)i;
        }
    }
}

void EntityRenderer::buildMeshParts(tinygltf::Model& gltf)
{
    for (size_t ni = 0; ni < gltf.nodes.size(); ni++) {
        auto& node = gltf.nodes[ni];
        if (node.mesh < 0) continue;

        auto& gltfMesh = gltf.meshes[node.mesh];
        for (auto& prim : gltfMesh.primitives) {
            auto posIt = prim.attributes.find("POSITION");
            auto uvIt = prim.attributes.find("TEXCOORD_0");
            if (posIt == prim.attributes.end()) continue;

            auto& posAcc = gltf.accessors[posIt->second];
            auto& posView = gltf.bufferViews[posAcc.bufferView];
            auto& posBuf = gltf.buffers[posView.buffer];
            const float* posData = reinterpret_cast<const float*>(
                posBuf.data.data() + posView.byteOffset + posAcc.byteOffset);

            std::vector<GLfloat> posVerts(posData, posData + posAcc.count * 3);

            std::vector<GLfloat> uvVerts;
            if (uvIt != prim.attributes.end()) {
                auto& uvAcc = gltf.accessors[uvIt->second];
                auto& uvView = gltf.bufferViews[uvAcc.bufferView];
                auto& uvBuf = gltf.buffers[uvView.buffer];
                const float* uvData = reinterpret_cast<const float*>(
                    uvBuf.data.data() + uvView.byteOffset + uvAcc.byteOffset);
                uvVerts.assign(uvData, uvData + uvAcc.count * 2);
            } else {
                uvVerts.resize(posAcc.count * 2, 0.0f);
            }

            std::vector<GLuint> indices;
            if (prim.indices >= 0) {
                auto& idxAcc = gltf.accessors[prim.indices];
                auto& idxView = gltf.bufferViews[idxAcc.bufferView];
                auto& idxBuf = gltf.buffers[idxView.buffer];
                const uint8_t* raw = idxBuf.data.data() + idxView.byteOffset + idxAcc.byteOffset;
                if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    const uint16_t* p = reinterpret_cast<const uint16_t*>(raw);
                    indices.assign(p, p + idxAcc.count);
                } else if (idxAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    const uint32_t* p = reinterpret_cast<const uint32_t*>(raw);
                    indices.assign(p, p + idxAcc.count);
                } else {
                    const uint8_t* p = raw;
                    indices.assign(p, p + idxAcc.count);
                }
            }

            Mesh mesh;
            mesh.vertexPositions = std::move(posVerts);
            mesh.textureCoords = std::move(uvVerts);
            mesh.indices = std::move(indices);

            Model* model = new Model();
            model->addData(mesh); // addData() already calls genVAO() internally

            GltfMeshPart part;
            part.model = model;
            part.nodeIndex = (int)ni;
            m_meshParts.push_back(part);
        }
    }
}

glm::mat4 EntityRenderer::getNodeWorldTransform(int nodeIdx, float animTime)
{
    // Walk from node to root, building the transform chain
    // Root nodes have parent -1
    glm::mat4 result(1.0f);

    // Collect nodes from leaf to root
    std::vector<int> chain;
    int cur = nodeIdx;
    while (cur >= 0) {
        chain.push_back(cur);
        cur = m_parentMap[cur];
    }

    // Apply from root to leaf
    for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
        int n = *it;
        auto& node = m_gltfModel.nodes[n];

        glm::mat4 local(1.0f);

        if (!node.matrix.empty()) {
            for (int c = 0; c < 4; c++)
                for (int r = 0; r < 4; r++)
                    local[c][r] = (float)node.matrix[c * 4 + r];
        } else {
            if (!node.translation.empty())
                local = glm::translate(local,
                    glm::vec3((float)node.translation[0],
                              (float)node.translation[1],
                              (float)node.translation[2]));
            if (!node.rotation.empty())
                local *= glm::mat4_cast(glm::quat(
                    (float)node.rotation[3], (float)node.rotation[0],
                    (float)node.rotation[1], (float)node.rotation[2]));
            if (!node.scale.empty())
                local = glm::scale(local,
                    glm::vec3((float)node.scale[0],
                              (float)node.scale[1],
                              (float)node.scale[2]));
        }

        // Check for animation targeting this node
        for (auto& anim : m_gltfModel.animations) {
            for (auto& ch : anim.channels) {
                if (ch.target_node != n) continue;
                if (ch.target_path != "rotation") continue;

                auto& sampler = anim.samplers[ch.sampler];
                auto& inputAcc = m_gltfModel.accessors[sampler.input];
                auto& outputAcc = m_gltfModel.accessors[sampler.output];

                auto& inView = m_gltfModel.bufferViews[inputAcc.bufferView];
                auto& outView = m_gltfModel.bufferViews[outputAcc.bufferView];
                auto& inBuf = m_gltfModel.buffers[inView.buffer];
                auto& outBuf = m_gltfModel.buffers[outView.buffer];

                const float* times = reinterpret_cast<const float*>(
                    inBuf.data.data() + inView.byteOffset + inputAcc.byteOffset);
                const float* values = reinterpret_cast<const float*>(
                    outBuf.data.data() + outView.byteOffset + outputAcc.byteOffset);

                float maxTime = times[inputAcc.count - 1];
                float t = std::fmod(animTime, maxTime);

                int k = 0;
                for (int i = 1; i < (int)inputAcc.count; i++) {
                    if (times[i] > t) { k = i - 1; break; }
                }
                int k2 = (k + 1) % inputAcc.count;
                float alpha = 0.0f;
                float dtVal = times[k2] - times[k];
                if (dtVal > 0.0001f) alpha = (t - times[k]) / dtVal;

                glm::quat q0(values[k2*4+3], values[k2*4+0],
                             values[k2*4+1], values[k2*4+2]);
                glm::quat q1(values[k*4+3], values[k*4+0],
                             values[k*4+1], values[k*4+2]);
                glm::quat q = glm::slerp(q0, q1, alpha);

                local *= glm::mat4_cast(q);
            }
        }

        result *= local;
    }

    return result;
}

void EntityRenderer::addEntity(const PigmanEntity& e)
{
    m_entities.push_back(&e);
}

void EntityRenderer::render(const Camera& camera)
{
    if (m_meshParts.empty() || m_entities.empty()) return;

    m_shader.useProgram();
    m_shader.loadProjectionViewMatrix(camera.getProjectionViewMatrix());
    m_texture.bindTexture();

    for (const auto* e : m_entities) {
        if (e->state == PigmanEntity::Dead && e->deathAnimTimer > 1.0f) continue;

        // Base world transform
        glm::mat4 worldMat(1.0f);
        worldMat = glm::translate(worldMat, e->position);
        worldMat = glm::rotate(worldMat, glm::radians(e->rotation.y + 180.0f),
                               glm::vec3(0, 1, 0));
        worldMat = glm::scale(worldMat, glm::vec3(0.6f)); // Zoglin ~2 units → ~1.2 blocks tall

        // Programmatic animation layers
        glm::mat4 animMat(1.0f);

        if (e->state == PigmanEntity::Attack) {
            float phase = 1.0f - (e->attackCooldown / 1.5f);
            if (phase < 0.3f) {
                float t = phase / 0.3f;
                float lunge = std::sin(t * 3.14159265f) * 0.3f;
                animMat = glm::translate(animMat, glm::vec3(0, 0, -lunge));
                float headDip = std::sin(t * 3.14159265f) * 15.0f;
                animMat = glm::rotate(animMat, glm::radians(headDip),
                                      glm::vec3(1, 0, 0));
            }
        }
        else if (e->state == PigmanEntity::Dead) {
            float t = e->deathAnimTimer;
            if (t > 1.0f) t = 1.0f;
            animMat = glm::rotate(animMat, glm::radians(t * 90.0f),
                                  glm::vec3(0, 0, 1));
            float s = 1.0f - t;
            animMat = glm::scale(animMat, glm::vec3(s, s, s));
        }

        // Tint: white for hurt, normal otherwise
        if (m_tintLocation >= 0) {
            if (e->state == PigmanEntity::Hurt) {
                glUniform3f(m_tintLocation, 3.0f, 3.0f, 3.0f);
            } else {
                glUniform3f(m_tintLocation, 1.0f, 1.0f, 1.0f);
            }
        }

        float animTime = e->stateTimer; // animate during patrol and chase

        // Render each mesh part with full hierarchy transform
        for (auto& part : m_meshParts) {
            glm::mat4 nodeMat = getNodeWorldTransform(part.nodeIndex, animTime);
            glm::mat4 finalMat = worldMat * animMat * nodeMat;
            m_shader.loadModelMatrix(finalMat);
            part.model->bindVAO();
            glDrawElements(GL_TRIANGLES, part.model->getIndicesCount(),
                          GL_UNSIGNED_INT, nullptr);
        }
    }

    m_entities.clear();
}
