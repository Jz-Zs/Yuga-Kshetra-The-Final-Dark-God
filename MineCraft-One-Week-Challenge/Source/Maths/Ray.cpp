#include "Ray.h"
#include <cmath>
#include <limits>

static constexpr float kInf = std::numeric_limits<float>::infinity();

Ray::Ray(const glm::vec3 &origin, const glm::vec3 &rotation)
    : m_origin(origin)
    , m_end(origin)
{
    // Spherical → Cartesian direction (unit vector)
    float yr = glm::radians(rotation.y);
    float pr = glm::radians(rotation.x);
    float cp = glm::cos(pr);
    m_dir = glm::vec3(
         glm::sin(yr) * cp,
        -glm::sin(pr),
        -glm::cos(yr) * cp
    );

    // Starting voxel
    m_voxel = glm::ivec3(
        static_cast<int>(std::floor(origin.x)),
        static_cast<int>(std::floor(origin.y)),
        static_cast<int>(std::floor(origin.z))
    );

    // Step direction per axis
    m_step.x = (m_dir.x > 0.0f) ? 1 : ((m_dir.x < 0.0f) ? -1 : 0);
    m_step.y = (m_dir.y > 0.0f) ? 1 : ((m_dir.y < 0.0f) ? -1 : 0);
    m_step.z = (m_dir.z > 0.0f) ? 1 : ((m_dir.z < 0.0f) ? -1 : 0);

    // tMax & tDelta per axis
    for (int i = 0; i < 3; i++) {
        float d = m_dir[i];
        if (d != 0.0f) {
            float boundary = static_cast<float>(m_voxel[i] + (m_step[i] > 0 ? 1 : 0));
            m_tMax[i] = (boundary - origin[i]) / d;
            m_tDelta[i] = 1.0f / std::abs(d);
        } else {
            m_tMax[i] = kInf;
            m_tDelta[i] = kInf;
        }
    }
}

bool Ray::advance() {
    // Pick axis with smallest tMax (tie-break: X < Y < Z)
    if (m_tMax.x < m_tMax.y) {
        if (m_tMax.x < m_tMax.z) {
            m_length = m_tMax.x;
            m_end = m_origin + m_dir * m_tMax.x;
            m_voxel.x += m_step.x;
            m_tMax.x += m_tDelta.x;
        } else {
            m_length = m_tMax.z;
            m_end = m_origin + m_dir * m_tMax.z;
            m_voxel.z += m_step.z;
            m_tMax.z += m_tDelta.z;
        }
    } else {
        if (m_tMax.y < m_tMax.z) {
            m_length = m_tMax.y;
            m_end = m_origin + m_dir * m_tMax.y;
            m_voxel.y += m_step.y;
            m_tMax.y += m_tDelta.y;
        } else {
            m_length = m_tMax.z;
            m_end = m_origin + m_dir * m_tMax.z;
            m_voxel.z += m_step.z;
            m_tMax.z += m_tDelta.z;
        }
    }
    return !std::isinf(m_length);
}
