#include "SpiderAI.h"
#include "../World/World.h"
#include "../Player/Player.h"
#include "../World/WorldConstants.h"
#include "../Entity/PigmanAI.h"  // Reuse findPath, hasLineOfSight, isWalkable
#include <cmath>
#include <cstdlib>

namespace SpiderAI {

static float randFloat(float lo, float hi) {
    float t = (float)std::rand() / (float)RAND_MAX;
    return lo + t * (hi - lo);
}

void update(SpiderEntity& e, float dt, Player& player, World& world) {
    glm::vec3 toPlayer = player.position - e.position;
    toPlayer.y += 0.6f;
    float distToPlayer = glm::length(glm::vec3(toPlayer.x, 0, toPlayer.z));

    e.meleeCooldown -= dt;
    e.rangedCooldown -= dt;
    e.freezeTimer -= dt;
    e.hurtTimer -= dt;
    e.aggroTimer -= dt;
    e.m_slowTimer -= dt;
    e.stateTimer += dt;
    float slowMul = (e.m_slowTimer > 0.0f) ? e.m_slowFactor : 1.0f;

    switch (e.state) {

    case SpiderEntity::Patrol: {
        bool isStopped = (glm::length(glm::vec2(e.velocity.x, e.velocity.z)) < 0.01f);
        float interval = isStopped ? 0.3f : randFloat(3.0f, 5.0f);
        if (e.stateTimer > interval) {
            e.stateTimer = 0.0f;
            float angle = randFloat(0.0f, 360.0f);
            if (e.position.x < 10) angle = randFloat(-60, 60);
            else if (e.position.x > MVP_WORLD_SIZE_X - 10) angle = randFloat(120, 240);
            if (e.position.z < 10) angle = randFloat(30, 150);
            else if (e.position.z > MVP_WORLD_SIZE_Z - 10) angle = randFloat(210, 330);
            float dx =  std::sin(glm::radians(angle));
            float dz = -std::cos(glm::radians(angle));
            e.rotation.y = glm::degrees(std::atan2(dx, dz));
            e.velocity.x = dx * 2.0f * 0.4f * slowMul;  // patrol speed 2.0
            e.velocity.z = dz * 2.0f * 0.4f * slowMul;
        }

        if (distToPlayer < 10.0f && PigmanAI::findPath(world, e.position, player.position).size() > 0) {
            // verify line of sight via ray-step check
            glm::vec3 dir = glm::normalize(player.position - e.position);
            bool blocked = false;
            for (float t = 0.5f; t < glm::distance(e.position, player.position); t += 0.5f) {
                glm::vec3 p = e.position + dir * t;
                int bx = (int)p.x, by = (int)p.y, bz = (int)p.z;
                auto block = world.getBlock(bx, by, bz);
                if (block.id != 0 && block.getData().isCollidable) { blocked = true; break; }
            }
            if (!blocked) {
                e.state = SpiderEntity::Chase;
                e.stateTimer = 0.0f;
                e.path.clear();
                e.stuckPosition = e.position;
                e.stuckTimer = 0.0f;
            }
        }
        break;
    }

    case SpiderEntity::Chase: {
        // RANGED MODE: kite at 5-8 blocks
        if (distToPlayer > 10.0f && e.aggroTimer <= 0.0f) {
            e.state = SpiderEntity::Patrol;
            e.stateTimer = 0.0f;
            e.path.clear();
            break;
        }

        // Switch to melee if player closes within 5 blocks
        if (distToPlayer <= 5.0f) {
            e.state = SpiderEntity::MeleeAttack;
            e.stateTimer = 0.0f;
            e.meleeCooldown = 0.3f;
            break;
        }

        // Ranged attack: fire projectile every 2s, LOS check
        if (e.rangedCooldown <= 0.0f) {
            glm::vec3 dir = glm::normalize(player.position - e.position);
            bool blocked = false;
            for (float t = 0.5f; t < distToPlayer; t += 0.5f) {
                glm::vec3 p = e.position + dir * t;
                auto b = world.getBlock((int)p.x, (int)p.y, (int)p.z);
                if (b.id != 0 && b.getData().isCollidable) { blocked = true; break; }
            }
            if (!blocked) {
                e.rangedCooldown = 2.0f;
                e.freezeTimer = 0.2f;
            }
        }

        float speedMul = (e.freezeTimer > 0.0f) ? 0.3f : 1.0f;

        // Kiting movement: maintain 5-8 block distance
        toPlayer.y = 0;
        float hDist = glm::length(toPlayer);
        glm::vec3 awayDir(0, 0, 0);
        if (hDist > 0.01f) {
            glm::vec3 toPlayerNorm = glm::normalize(toPlayer);
            if (hDist < 6.5f) {
                // Too close — back away from player
                awayDir = -toPlayerNorm;
            } else if (hDist > 7.5f) {
                // Too far — move toward player
                awayDir = toPlayerNorm;
            }
            // else 6.5-7.5: hold position (side-strafe occasionally)
        }

        e.velocity.x = awayDir.x * e.moveSpeed * 0.625f * speedMul * slowMul;
        e.velocity.z = awayDir.z * e.moveSpeed * speedMul * slowMul;

        // Always face player
        {
            float targetYaw = glm::degrees(std::atan2(toPlayer.x, toPlayer.z));
            float diff = targetYaw - e.rotation.y;
            while (diff > 180) diff -= 360;
            while (diff < -180) diff += 360;
            e.rotation.y += diff * std::min(dt * 4.0f, 1.0f);
        }
        break;
    }

    case SpiderEntity::MeleeAttack: {
        if (distToPlayer > 5.0f) {
            e.state = SpiderEntity::Chase;
            e.stateTimer = 0.0f;
            break;
        }
        // Chase toward player, don't stop until within melee range
        if (distToPlayer > 2.0f) {
            toPlayer.y = 0;
            if (glm::length(toPlayer) > 0.1f) {
                glm::vec3 d = glm::normalize(toPlayer);
                e.velocity.x = d.x * e.moveSpeed * 0.625f * slowMul;
                e.velocity.z = d.z * e.moveSpeed * slowMul;
            }
            // Smooth rotation
            float targetYaw = glm::degrees(std::atan2(toPlayer.x, toPlayer.z));
            float diff = targetYaw - e.rotation.y;
            while (diff > 180) diff -= 360;
            while (diff < -180) diff += 360;
            e.rotation.y += diff * std::min(dt * 4.0f, 1.0f);
            break;
        }
        // Within 2 blocks — stand and attack
        e.velocity.x = 0; e.velocity.z = 0;
        // Actual damage in World::updateEntities
        break;
    }

    case SpiderEntity::Hurt: {
        e.velocity.x = 0; e.velocity.z = 0;
        if (e.hurtTimer <= 0.0f) {
            e.state = SpiderEntity::Chase;
            e.stateTimer = 0.0f;
        }
        break;
    }

    case SpiderEntity::Dead: {
        e.velocity.x = 0; e.velocity.z = 0;
        break;
    }
    }
}

} // namespace SpiderAI
