#include "PigmanAI.h"
#include "../World/World.h"
#include "../Player/Player.h"
#include "../World/WorldConstants.h"
#include <iostream>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <cstdlib>

namespace PigmanAI {

// --- Utility: Random float ---
static float randFloat(float lo, float hi) {
    float t = (float)std::rand() / (float)RAND_MAX;
    return lo + t * (hi - lo);
}

// --- Hash for ivec3 ---
struct IVec3Hash {
    size_t operator()(const glm::ivec3& v) const {
        size_t h = 17;
        h = h * 31 + std::hash<int>()(v.x);
        h = h * 31 + std::hash<int>()(v.y);
        h = h * 31 + std::hash<int>()(v.z);
        return h;
    }
};

// --- Check if a world position is walkable (ground solid, body+head clear) ---
static bool isWalkable(World& world, int x, int y, int z) {
    if (x < 0 || x >= MVP_WORLD_SIZE_X) return false;
    if (z < 0 || z >= MVP_WORLD_SIZE_Z) return false;
    if (y <= 0 || y >= MVP_WORLD_HEIGHT) return false;

    auto ground = world.getBlock(x, y - 1, z);
    if (ground.id == 0 || !ground.getData().isCollidable) return false;

    auto body = world.getBlock(x, y, z);
    if (body.getData().isCollidable && body.id != 0) return false;

    auto head = world.getBlock(x, y + 1, z);
    if (head.getData().isCollidable && head.id != 0) return false;

    return true;
}

// --- Find ground Y for a given (x,z) ---
static int getGroundY(World& world, int x, int z) {
    for (int y = MVP_WORLD_HEIGHT - 1; y > 0; y--) {
        auto block = world.getBlock(x, y, z);
        if (block.id != 0 && block.getData().isCollidable) {
            return y + 1;
        }
    }
    return -1;
}

// --- Line of sight check ---
static bool hasLineOfSight(World& world, const glm::vec3& from, const glm::vec3& to) {
    glm::vec3 dir = glm::normalize(to - from);
    float dist = glm::distance(from, to);
    for (float t = 0.0f; t < dist; t += 0.5f) {
        glm::vec3 p = from + dir * t;
        int x = static_cast<int>(p.x);
        int y = static_cast<int>(p.y);
        int z = static_cast<int>(p.z);
        auto block = world.getBlock(x, y, z);
        if (block.id != 0 && block.getData().isCollidable)
            return false;
    }
    return true;
}

// --- A* Pathfinding ---
std::vector<glm::ivec3> findPath(World& world,
                                  const glm::vec3& from,
                                  const glm::vec3& to) {
    glm::ivec3 start(static_cast<int>(from.x),
                      static_cast<int>(from.y),
                      static_cast<int>(from.z));
    glm::ivec3 goal(static_cast<int>(to.x),
                     static_cast<int>(to.y),
                     static_cast<int>(to.z));

    int sy = getGroundY(world, start.x, start.z);
    int gy = getGroundY(world, goal.x, goal.z);
    if (sy < 0 || gy < 0) return {};
    start.y = sy;
    goal.y = gy;

    if (!isWalkable(world, start.x, start.y, start.z)) return {};

    struct Node { float g = 0, h = 0; glm::ivec3 parent{-1,-1,-1}; bool closed = false; };
    std::unordered_map<glm::ivec3, Node, IVec3Hash> nodes;
    nodes[start].g = 0;
    nodes[start].h = std::abs((float)(start.x - goal.x)) +
                     std::abs((float)(start.y - goal.y)) +
                     std::abs((float)(start.z - goal.z));
    nodes[start].parent = {-1, -1, -1};

    auto cmp = [&](const glm::ivec3& a, const glm::ivec3& b) {
        return nodes[a].g + nodes[a].h > nodes[b].g + nodes[b].h;
    };
    std::priority_queue<glm::ivec3, std::vector<glm::ivec3>, decltype(cmp)> open(cmp);
    open.push(start);

    int steps = 0;
    while (!open.empty() && steps < 256) {
        steps++;
        glm::ivec3 cur = open.top(); open.pop();
        if (nodes[cur].closed) continue;
        nodes[cur].closed = true;

        // Reached goal or adjacent
        int dx = std::abs(cur.x - goal.x);
        int dz = std::abs(cur.z - goal.z);
        if ((dx <= 1 && dz <= 1) || (cur == goal)) {
            std::vector<glm::ivec3> path;
            glm::ivec3 p = cur;
            while (p != glm::ivec3(-1, -1, -1)) {
                path.push_back(p);
                p = nodes[p].parent;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        glm::ivec3 nb[4] = {
            {cur.x+1, cur.y, cur.z}, {cur.x-1, cur.y, cur.z},
            {cur.x, cur.y, cur.z+1}, {cur.x, cur.y, cur.z-1},
        };

        for (auto& n : nb) {
            int gy2 = getGroundY(world, n.x, n.z);
            if (gy2 < 0) continue;
            n.y = gy2;
            if (!isWalkable(world, n.x, n.y, n.z)) continue;

            float stepCost = (n.y != cur.y) ? 1.5f : 1.0f;
            float newG = nodes[cur].g + stepCost;
            auto it = nodes.find(n);
            if (it != nodes.end() && it->second.closed) continue;
            if (it != nodes.end() && newG >= it->second.g) continue;

            nodes[n].g = newG;
            nodes[n].h = std::abs((float)(n.x - goal.x)) +
                         std::abs((float)(n.y - goal.y)) +
                         std::abs((float)(n.z - goal.z));
            nodes[n].parent = cur;
            open.push(n);
        }
    }
    return {};
}

// --- AI State Machine ---
void update(PigmanEntity& e, float dt, Player& player, World& world) {
    glm::vec3 toPlayer = player.position - e.position;
    toPlayer.y += 0.6f; // aim at player upper body
    float distToPlayer = glm::length(glm::vec3(toPlayer.x, 0, toPlayer.z));

    e.attackCooldown -= dt;
    e.hurtTimer -= dt;
    e.aggroTimer -= dt;
    e.m_slowTimer -= dt;
    e.stateTimer += dt;
    float slowMul = (e.m_slowTimer > 0.0f) ? e.m_slowFactor : 1.0f;

    switch (e.state) {

    case PigmanEntity::Patrol: {
        // First direction after 0.3s pause, then new direction every 3-5s
        bool isStopped = (glm::length(glm::vec2(e.velocity.x, e.velocity.z)) < 0.01f);
        float interval = isStopped ? 0.3f : randFloat(3.0f, 5.0f);
        if (e.stateTimer > interval) {
            e.stateTimer = 0.0f;
            float angle = randFloat(0.0f, 360.0f);
            if (e.position.x < 10) angle = randFloat(-60, 60);
            else if (e.position.x > MVP_WORLD_SIZE_X - 10) angle = randFloat(120, 240);
            if (e.position.z < 10) angle = randFloat(30, 150);
            else if (e.position.z > MVP_WORLD_SIZE_Z - 10) angle = randFloat(210, 330);
            // direction in game coords: forward = (sin(angle), -cos(angle))
            float dx =  std::sin(glm::radians(angle));
            float dz = -std::cos(glm::radians(angle));
            e.rotation.y = glm::degrees(std::atan2(dx, dz)); // match chase convention
            e.velocity.x = dx * e.moveSpeed * 0.4f * slowMul;
            e.velocity.z = dz * e.moveSpeed * 0.4f * slowMul;
        }

        if (distToPlayer < 8.0f && hasLineOfSight(world, e.position, player.position)) {
            e.state = PigmanEntity::Chase;
            e.stateTimer = 0.0f;
            e.path.clear();
            e.stuckPosition = e.position;
            e.stuckTimer = 0.0f;
            break;
        }
        break;
    }

    case PigmanEntity::Chase: {
        if (e.stateTimer > 0.5f || e.path.empty()) {
            e.path = findPath(world, e.position, player.position);
            e.pathIndex = 0;
            e.stateTimer = 0.0f;
        }

        // Stuck detection
        float moved = glm::distance(
            glm::vec3(e.position.x, 0, e.position.z),
            glm::vec3(e.stuckPosition.x, 0, e.stuckPosition.z));
        if (moved < 0.1f) { e.stuckTimer += dt; }
        else { e.stuckTimer = 0.0f; e.stuckPosition = e.position; }

        bool lostTarget = (distToPlayer > 8.0f && e.aggroTimer <= 0.0f);
        bool stuck = (e.stuckTimer > 3.0f);
        if (lostTarget || stuck) {
            e.state = PigmanEntity::Patrol;
            e.stateTimer = 0.0f;
            e.stuckTimer = 0.0f;
            e.path.clear();
            break;
        }

        if (distToPlayer < 2.0f) {
            e.state = PigmanEntity::Attack;
            e.attackCooldown = 0.2f; // brief delay before first hit
            e.velocity.x = 0; e.velocity.z = 0;
            break;
        }

        if (!e.path.empty() && e.pathIndex < (int)e.path.size()) {
            glm::vec3 target(e.path[e.pathIndex].x + 0.5f, 0,
                             e.path[e.pathIndex].z + 0.5f);
            glm::vec3 d = target - e.position; d.y = 0;
            float hd = glm::length(d);
            if (hd < 0.3f) { e.pathIndex++; }
            else {
                d = glm::normalize(d);
                e.velocity.x = d.x * e.moveSpeed * 0.625f * slowMul;
                e.velocity.z = d.z * e.moveSpeed * slowMul;
            }
        } else {
            // No path — move directly toward player
            toPlayer.y = 0;
            if (glm::length(toPlayer) > 0.1f) {
                glm::vec3 d = glm::normalize(toPlayer);
                e.velocity.x = d.x * e.moveSpeed * 0.625f * slowMul;
                e.velocity.z = d.z * e.moveSpeed * slowMul;
            }
        }
        // Smooth rotation toward player
        {
            float targetYaw = glm::degrees(std::atan2(toPlayer.x, toPlayer.z));
            float diff = targetYaw - e.rotation.y;
            while (diff > 180) diff -= 360;
            while (diff < -180) diff += 360;
            e.rotation.y += diff * std::min(dt * 3.0f, 1.0f); // slower chase turn
        }
        break;
    }

    case PigmanEntity::Attack: {
        e.velocity.x = 0; e.velocity.z = 0;
        // rotation stays at last chase direction

        if (distToPlayer > 2.0f) {
            e.state = PigmanEntity::Chase;
            e.stateTimer = 0.0f;
            break;
        }
        // Actual damage dealt in World::updateEntities when cooldown ≤ 0
        break;
    }

    case PigmanEntity::Hurt: {
        e.velocity.x = 0; e.velocity.z = 0;
        if (e.hurtTimer <= 0.0f) {
            e.state = PigmanEntity::Chase;
            e.stateTimer = 0.0f;
        }
        break;
    }

    case PigmanEntity::Dead: {
        e.velocity.x = 0; e.velocity.z = 0;
        // deathAnimTimer incremented in World::updateEntities
        break;
    }
    }
}

} // namespace PigmanAI
