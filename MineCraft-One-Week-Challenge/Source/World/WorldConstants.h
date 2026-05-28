#ifndef WORLDCONSTANTS_H_INCLUDED
#define WORLDCONSTANTS_H_INCLUDED

// Defines the most basic rules for chunk generation in any given world.

constexpr int CHUNK_SIZE = 16, CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE,
              CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE,

              WATER_LEVEL = 64;

// MVP 128x128x64 world bounds
constexpr int MVP_WORLD_SIZE_X  = 128;
constexpr int MVP_WORLD_SIZE_Z  = 128;
constexpr int MVP_WORLD_HEIGHT  = 64;
constexpr int MVP_WATER_LEVEL   = 30;
constexpr int MVP_CHUNK_COUNT   = MVP_WORLD_SIZE_X / CHUNK_SIZE; // 8

#endif // WORLDCONSTANTS_H_INCLUDED
