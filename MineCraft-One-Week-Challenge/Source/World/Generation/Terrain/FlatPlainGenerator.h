#ifndef FLATPLAINGENERATOR_H_INCLUDED
#define FLATPLAINGENERATOR_H_INCLUDED

#include "TerrainGenerator.h"
#include "../../../Maths/NoiseGenerator.h"
#include "../../../Util/Random.h"

class Chunk;

class FlatPlainGenerator : public TerrainGenerator {
  public:
    FlatPlainGenerator();

    void generateTerrainFor(Chunk &chunk) override;
    int getMinimumSpawnHeight() const noexcept override;

  private:
    NoiseGenerator m_heightNoise;
    Random<std::minstd_rand> m_random;
};

#endif // FLATPLAINGENERATOR_H_INCLUDED
