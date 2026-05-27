#pragma once

#include <SFML/Window/Window.hpp>
#include <SFML/System/Time.hpp>

namespace GUI
{
    [[nodiscard]] bool init(sf::Window* window);

    void begin_frame(sf::Window& window, sf::Time dt);

    void shutdown();
    void render(sf::Window& window);

    void event(const sf::Window& window, sf::Event& e);
} // namespace GUI
