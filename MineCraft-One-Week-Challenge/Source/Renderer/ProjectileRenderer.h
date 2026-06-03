#ifndef PROJECTILERENDERER_H_INCLUDED
#define PROJECTILERENDERER_H_INCLUDED

#include <vector>
#include <glad/glad.h>
#include "../Maths/glm.h"
#include "../Shaders/BasicShader.h"
#include "../Texture/TextureAtlas.h"

class Camera;
struct SpiderProjectile;

class ProjectileRenderer {
public:
    ProjectileRenderer(int cellX = 13, int cellY = 2, bool rotate90 = false);
    ~ProjectileRenderer();

    void addProjectile(const SpiderProjectile& p);
    void addPosition(const glm::vec3& pos, float scale = 1.0f);
    void render(const Camera& camera);

private:
    void buildQuad(int cellX, int cellY, bool rotate90);

    struct BatchEntry { glm::vec3 pos; float scale = 1.0f; };
    BasicShader m_shader;
    TextureAtlas m_atlas;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    std::vector<BatchEntry> m_entries;
};

#endif
