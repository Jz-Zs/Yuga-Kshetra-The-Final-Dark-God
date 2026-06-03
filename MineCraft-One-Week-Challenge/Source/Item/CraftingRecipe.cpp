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

    // 橡木+橡木+木棍(中竖) → 1 木剑
    g_recipes.push_back({{
        nullptr, &Material::OAK_BARK_BLOCK, nullptr,
        nullptr, &Material::OAK_BARK_BLOCK, nullptr,
        nullptr, &Material::STICK,           nullptr,
    }, &Material::WOODEN_SWORD, 1});

    // === 入口7 新增配方 ===

    // 3 石头(左竖) → 1 圆石
    g_recipes.push_back({{
        &Material::STONE_BLOCK, nullptr, nullptr,
        &Material::STONE_BLOCK, nullptr, nullptr,
        &Material::STONE_BLOCK, nullptr, nullptr,
    }, &Material::COBBLESTONE, 1});

    // 木镐: 3橡木(顶行) + 2木棍(中下竖)
    g_recipes.push_back({{
        &Material::OAK_BARK_BLOCK, &Material::OAK_BARK_BLOCK, &Material::OAK_BARK_BLOCK,
        nullptr,                   &Material::STICK,           nullptr,
        nullptr,                   &Material::STICK,           nullptr,
    }, &Material::WOODEN_PICKAXE, 1});

    // 石镐: 3圆石(顶行) + 2木棍(中下竖)
    g_recipes.push_back({{
        &Material::COBBLESTONE, &Material::COBBLESTONE, &Material::COBBLESTONE,
        nullptr,                &Material::STICK,       nullptr,
        nullptr,                &Material::STICK,       nullptr,
    }, &Material::STONE_PICKAXE, 1});

    // 铁镐: 3铁锭(顶行) + 2木棍(中下竖)
    g_recipes.push_back({{
        &Material::IRON_INGOT, &Material::IRON_INGOT, &Material::IRON_INGOT,
        nullptr,               &Material::STICK,      nullptr,
        nullptr,               &Material::STICK,      nullptr,
    }, &Material::IRON_PICKAXE, 1});

    // 木斧: 3橡木(左上L) + 2木棍(中下竖)
    g_recipes.push_back({{
        &Material::OAK_BARK_BLOCK, &Material::OAK_BARK_BLOCK, nullptr,
        &Material::OAK_BARK_BLOCK, &Material::STICK,          nullptr,
        nullptr,                   &Material::STICK,          nullptr,
    }, &Material::WOODEN_AXE, 1});

    // 石斧: 3圆石(左上L) + 2木棍(中下竖)
    g_recipes.push_back({{
        &Material::COBBLESTONE, &Material::COBBLESTONE, nullptr,
        &Material::COBBLESTONE, &Material::STICK,       nullptr,
        nullptr,                &Material::STICK,       nullptr,
    }, &Material::STONE_AXE, 1});

    // 铁斧: 3铁锭(左上L) + 2木棍(中下竖)
    g_recipes.push_back({{
        &Material::IRON_INGOT, &Material::IRON_INGOT, nullptr,
        &Material::IRON_INGOT, &Material::STICK,      nullptr,
        nullptr,               &Material::STICK,      nullptr,
    }, &Material::IRON_AXE, 1});

    // 铁剑: 铁锭+铁锭+木棍(中竖)
    g_recipes.push_back({{
        nullptr,               &Material::IRON_INGOT, nullptr,
        nullptr,               &Material::IRON_INGOT, nullptr,
        nullptr,               &Material::STICK,      nullptr,
    }, &Material::IRON_SWORD, 1});

    // 石箭矢: 圆石+木棍+木棍(左上到右下对角线)
    g_recipes.push_back({{
        &Material::COBBLESTONE, nullptr,               nullptr,
        nullptr,                &Material::STICK,       nullptr,
        nullptr,                nullptr,               &Material::STICK,
    }, &Material::STONE_ARROW, 4});

    // 8 Cobblestone ring → Furnace
    g_recipes.push_back({{
        &Material::COBBLESTONE, &Material::COBBLESTONE, &Material::COBBLESTONE,
        &Material::COBBLESTONE, nullptr,                &Material::COBBLESTONE,
        &Material::COBBLESTONE, &Material::COBBLESTONE, &Material::COBBLESTONE,
    }, &Material::FURNACE, 1});

    // 3 Silk (left column) → 1 Silk Thread
    g_recipes.push_back({{
        &Material::SILK, nullptr, nullptr,
        &Material::SILK, nullptr, nullptr,
        &Material::SILK, nullptr, nullptr,
    }, &Material::SILK_THREAD, 1});
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
