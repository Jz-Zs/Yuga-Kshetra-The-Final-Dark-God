#include "CraftingRecipe.h"
#include "ItemStack.h"

std::vector<CraftingRecipe> g_recipes;

void initCraftingRecipes()
{
    // 3 橡木(左列) → 1 木棍
    g_recipes.push_back({{
        &Material::OAK_BARK_BLOCK, nullptr, nullptr,
        &Material::OAK_BARK_BLOCK, nullptr, nullptr,
        &Material::OAK_BARK_BLOCK, nullptr, nullptr,
    }, &Material::STICK, 1});

    // 3 木棍(左列) → 1 木剑
    g_recipes.push_back({{
        &Material::STICK, nullptr, nullptr,
        &Material::STICK, nullptr, nullptr,
        &Material::STICK, nullptr, nullptr,
    }, &Material::WOODEN_SWORD, 1});
}

const CraftingRecipe* findMatchingRecipe(const ItemStack grid[9])
{
    for (const auto& recipe : g_recipes) {
        bool match = true;
        for (int i = 0; i < 9; i++) {
            const Material* expected = recipe.pattern[i];
            const Material::ID actualID = grid[i].getMaterial().id;
            if (expected == nullptr) {
                if (actualID != Material::ID::Nothing) {
                    match = false;
                    break;
                }
            } else {
                if (actualID != expected->id) {
                    match = false;
                    break;
                }
            }
        }
        if (match) return &recipe;
    }
    return nullptr;
}
