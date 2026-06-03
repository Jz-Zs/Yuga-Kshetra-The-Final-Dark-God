#include "SmeltingRecipe.h"
#include "ItemStack.h"

std::vector<SmeltingRecipe> g_smeltingRecipes;

void initSmeltingRecipes()
{
    g_smeltingRecipes.clear();

    // 3铁矿 + 3原木 → 1铁锭，5秒
    g_smeltingRecipes.push_back({
        Material::ID::IronOre, 3,
        Material::ID::OakBark, 3,
        Material::ID::IronIngot, 1
    });

    // 1生肉 + 1原木 → 1熟肉，5秒
    g_smeltingRecipes.push_back({
        Material::ID::RawMeat, 1,
        Material::ID::OakBark, 1,
        Material::ID::CookedMeat, 1
    });
}

const SmeltingRecipe* findSmeltingRecipe(Material::ID inputId, int inputCount,
                                          Material::ID fuelId, int fuelCount)
{
    for (const auto& r : g_smeltingRecipes) {
        if (r.input == inputId && inputCount >= r.inputCount &&
            r.fuel == fuelId && fuelCount >= r.fuelCount) {
            return &r;
        }
    }
    return nullptr;
}
