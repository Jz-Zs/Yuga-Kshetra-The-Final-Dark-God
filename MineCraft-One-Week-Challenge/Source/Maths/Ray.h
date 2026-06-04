#ifndef RAY_H_INCLUDED
#define RAY_H_INCLUDED

#include "glm.h"

/// @brief DDA (Amanatides-Woo) voxel traversal ray.
/// Construct with origin + player rotation (pitch, yaw, _).
/// Call advance() to step to the next voxel boundary.
class Ray {
  public:
    Ray(const glm::vec3 &origin, const glm::vec3 &rotation);

    /// Advance to next voxel along the ray. Returns false if stalled
    /// (direction component zero — should not happen with valid input).
    bool advance();

    /// Current voxel coordinates (integer block position).
    glm::ivec3 currentVoxel() const { return m_voxel; }

    /// Exact position on the ray at last boundary crossing.
    const glm::vec3 &getEnd() const { return m_end; }

    /// Total distance traveled along the ray so far.
    float getLength() const { return m_length; }

  private:
    glm::vec3 m_dir;          // normalized direction
    glm::vec3 m_origin;       // ray start
    glm::ivec3 m_voxel;       // current voxel coords
    glm::ivec3 m_step;        // +1 or -1 per axis
    glm::vec3 m_tMax;         // distance to next voxel boundary per axis
    glm::vec3 m_tDelta;       // distance to cross one voxel per axis
    glm::vec3 m_end;          // position at last boundary (starts at origin)
    float m_length = 0.0f;    // distance traveled
};

#endif // RAY_H_INCLUDED
