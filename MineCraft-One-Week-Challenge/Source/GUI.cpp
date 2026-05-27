#include "GUI.h"

#include <print>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_sfml/imgui-SFML.h>

namespace GUI
{
    bool init(sf::Window* window)
    {
        if (!ImGui::SFML::Init(*window, sf::Vector2f{window->getSize()}))
            return false;
        ImGui::GetIO().Fonts->AddFontDefault();
        return true;
    }

    void begin_frame(sf::Window& window, sf::Time dt)
    {
        ImGui::SFML::SetCurrentWindow(window);
        auto mousePos = sf::Mouse::getPosition(window);
        auto size = window.getSize();
        ImGui::SFML::Update(mousePos, sf::Vector2f(size), dt);
    }

    void shutdown()
    {
        ImGui::SFML::Shutdown();
    }

    void render(sf::Window& window)
    {
        ImGui::SFML::SetCurrentWindow(window);
        ImGui::SFML::Render();
    }

    void event(const sf::Window& window, sf::Event& e)
    {
        ImGui::SFML::ProcessEvent(window, e);
    }

} // namespace GUI
