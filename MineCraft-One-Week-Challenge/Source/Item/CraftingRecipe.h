#pragma once

#include "Material.h"
#include <vector>

class ItemStack;

struct CraftingRecipe {
    const Material* pattern[9];  // 3×3 grid, nullptr = empty
    const Material* output;
    int outputCount;
};

extern std::vector<CraftingRecipe> g_recipes;

const CraftingRecipe* findMatchingRecipe(const ItemStack grid[9]);
void initCraftingRecipes();
