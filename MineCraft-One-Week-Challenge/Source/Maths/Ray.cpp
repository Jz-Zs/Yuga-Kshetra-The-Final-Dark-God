#include "Ray.h"

#include <cmath>

Ray::Ray(const glm::vec3 &position, const glm::vec3 &direction)
    : m_rayStart(position)
    , m_rayEnd(position)
    , m_direction(direction)
{
}

void Ray::step(float scale)
{
    float yaw = glm::radians(m_direction.y + 90);
    float pitch = glm::radians(m_direction.x);

    auto &p = m_rayEnd;

    float dx = -glm::cos(yaw);
    float dy = -glm::tan(pitch);
    float dz = -glm::sin(yaw);

    // Normalize so step length = scale, no overshoot on steep angles
    float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    float inv = scale / len;

    p.x += dx * inv;
    p.y += dy * inv;
    p.z += dz * inv;
}

const glm::vec3 &Ray::getEnd() const
{
    return m_rayEnd;
}

float Ray::getLength() const
{
    return glm::distance(m_rayStart, m_rayEnd);
}
