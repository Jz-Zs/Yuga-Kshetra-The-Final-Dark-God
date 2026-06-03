#include "ProjectileRenderer.h"
#include "../Entity/SpiderProjectile.h"
#include "../Camera.h"
#include <glm/gtc/matrix_transform.hpp>

ProjectileRenderer::ProjectileRenderer()
    : m_shader("Basic", "Basic")
    , m_atlas("DefaultPack")
{
    buildQuad();
}

ProjectileRenderer::~ProjectileRenderer()
{
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
}

void ProjectileRenderer::buildQuad()
{
    // UV for cell (13, 2) in 16x16 atlas
    float cellU = 13.0f / 16.0f;
    float cellV = 2.0f / 16.0f;
    float u0 = cellU, u1 = cellU + 1.0f/16.0f;
    float v0 = cellV, v1 = cellV + 1.0f/16.0f;

    float vertices[] = {
        // pos (x,y,z)      // uv
        -0.15f,  0.15f, 0,  u0, v0,
         0.15f,  0.15f, 0,  u1, v0,
         0.15f, -0.15f, 0,  u1, v1,
        -0.15f, -0.15f, 0,  u0, v1,
    };
    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // uv (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void ProjectileRenderer::addProjectile(const SpiderProjectile& p)
{
    m_positions.push_back(p.position);
}

void ProjectileRenderer::render(const Camera& camera)
{
    if (m_positions.empty()) return;

    m_shader.useProgram();
    m_shader.loadProjectionViewMatrix(camera.getProjectionViewMatrix());
    m_atlas.bindTexture();

    glBindVertexArray(m_vao);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& pos : m_positions) {
        glm::mat4 model(1.0f);
        model = glm::translate(model, pos);

        // Billboard Y-rotation toward camera (camera.position is Entity::position)
        glm::vec3 toCam = camera.position - pos;
        float yaw = glm::degrees(std::atan2(toCam.x, toCam.z));
        model = glm::rotate(model, glm::radians(yaw), glm::vec3(0, 1, 0));

        m_shader.loadModelMatrix(model);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    }

    glDisable(GL_BLEND);
    glBindVertexArray(0);

    m_positions.clear();
}
