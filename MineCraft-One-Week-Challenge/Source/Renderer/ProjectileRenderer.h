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
    ProjectileRenderer();
    ~ProjectileRenderer();

    void addProjectile(const SpiderProjectile& p);
    void render(const Camera& camera);

private:
    void buildQuad();

    BasicShader m_shader;
    TextureAtlas m_atlas;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    std::vector<glm::vec3> m_positions;
};

#endif
