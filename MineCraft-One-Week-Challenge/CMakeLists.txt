cmake_minimum_required(VERSION 3.10)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    include("$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
endif()

project(
    minecraft-one-week-challenge
    VERSION 1.0
)

add_executable(${PROJECT_NAME}
    Source/Item/Material.cpp
    Source/Item/ItemStack.cpp
    Source/Item/CraftingRecipe.cpp
    Source/Application.cpp
    Source/World/Event/PlayerDigEvent.cpp
    Source/World/Block/BlockDatabase.cpp
    Source/World/Block/BlockData.cpp
    Source/World/Block/ChunkBlock.cpp
    Source/World/Block/BlockTypes/BlockType.cpp
    Source/World/World.cpp
    Source/World/Generation/Biome/LightForest.cpp
    Source/World/Generation/Biome/Biome.cpp
    Source/World/Generation/Biome/DesertBiome.cpp
    Source/World/Generation/Biome/GrasslandBiome.cpp
    Source/World/Generation/Biome/TemperateForestBiome.cpp
    Source/World/Generation/Biome/OceanBiome.cpp
    Source/World/Generation/Structures/TreeGenerator.cpp
    Source/World/Generation/Structures/StructureBuilder.cpp
    Source/World/Generation/Terrain/SuperFlatGenerator.cpp
    Source/World/Generation/Terrain/ClassicOverWorldGenerator.cpp
    Source/World/Generation/Terrain/FlatPlainGenerator.cpp
    Source/World/Chunk/ChunkMesh.cpp
    Source/World/Chunk/ChunkManager.cpp
    Source/World/Chunk/Chunk.cpp
    Source/World/Chunk/ChunkSection.cpp
    Source/World/Chunk/ChunkMeshBuilder.cpp
    Source/Player/Player.cpp
    Source/Maths/Ray.cpp
    Source/Maths/Frustum.cpp
    Source/Maths/NoiseGenerator.cpp
    Source/Maths/Vector2XZ.cpp
    Source/Maths/Matrix.cpp
    Source/Maths/GeneralMaths.cpp
    Source/Camera.cpp
    Source/GL/GLFunctions.cpp
    Source/Texture/CubeTexture.cpp
    Source/Texture/BasicTexture.cpp
    Source/Texture/TextureAtlas.cpp
    Source/Input/ToggleKey.cpp
    Source/Input/Keyboard.cpp
    Source/Main.cpp
    Source/Controller.cpp
    Source/Util/Random.cpp
    Source/Util/FileUtil.cpp
    Source/Util/BitmapText.cpp
    Source/Shaders/FloraShader.cpp
    Source/Shaders/WaterShader.cpp
    Source/Shaders/SkyboxShader.cpp
    Source/Shaders/Shader.cpp
    Source/Shaders/ChunkShader.cpp
    Source/Shaders/BasicShader.cpp
    Source/Shaders/ShaderLoader.cpp
    Source/Renderer/RenderMaster.cpp
    Source/Renderer/WaterRenderer.cpp
    Source/Renderer/ChunkRenderer.cpp
    Source/Renderer/SkyboxRenderer.cpp
    Source/Renderer/FloraRenderer.cpp
    Source/Renderer/EntityRenderer.cpp
    Source/Entity/PigmanAI.cpp
    Source/Model.cpp
    Source/Util/Profiler.cpp
    Source/GUI.cpp
    Source/GL/GLUtils.cpp
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)

target_compile_definitions(${PROJECT_NAME} PRIVATE GLM_ENABLE_EXPERIMENTAL)

if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
    set(CMAKE_CXX_FLAGS_RELEASE "/Ox")
  	target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
  	target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
else()
  	target_compile_options(${PROJECT_NAME} PRIVATE 
		-Wall -Wextra -pedantic)		
endif()

find_package(glm CONFIG REQUIRED)
find_package(SFML COMPONENTS System Audio Network Window Graphics CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(ImGui-SFML CONFIG REQUIRED)

add_subdirectory(deps/glad)
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    deps
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SFML::System SFML::Audio SFML::Network SFML::Graphics SFML::Window
    imgui::imgui
    glm::glm
    ImGui-SFML::ImGui-SFML
    glad
)

# Copy runtime resources to executable directory for double-click launch
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/Shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Res
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/Res
)
