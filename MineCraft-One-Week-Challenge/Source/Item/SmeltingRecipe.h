#pragma once

#include "Material.h"
#include <vector>

class ItemStack;

struct SmeltingRecipe {
    Material::ID input;
    int inputCount;
    Material::ID fuel;
    int fuelCount;
    Material::ID output;
    int outputCount;
};

extern std::vector<SmeltingRecipe> g_smeltingRecipes;

void initSmeltingRecipes();

/// Returns the first recipe whose input/fuel match the given items and counts.
/// Returns nullptr if no recipe matches.
const SmeltingRecipe* findSmeltingRecipe(Material::ID inputId, int inputCount,
                                          Material::ID fuelId, int fuelCount);
