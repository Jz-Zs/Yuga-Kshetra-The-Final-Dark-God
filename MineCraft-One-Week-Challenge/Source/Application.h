#pragma once

#include <SFML/Audio/Music.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Window.hpp>

#include "Input/Keyboard.h"
#include "Input/ToggleKey.h"
#include "Player/Player.h"
#include "World/Chunk/Chunk.h"
#include "World/World.h"
#include "Renderer/RenderMaster.h"
#include "Camera.h"
float extern g_timeElapsed;


class Keyboard;

class Application
{
  public:
    Application(sf::Window& window, const Config& config);

    void on_event(const sf::Event& event);
    void on_update(const Keyboard& keyboard, sf::Time dt);
    void on_render(bool show_debug_info);

  private:
    sf::Window& m_window;
    const Config& m_config;
    RenderMaster m_masterRenderer;
    Camera m_camera;
    Player m_player;
    World m_world;
    bool m_prevLeftPressed = false;
    sf::Clock m_rightClickTimer;

    // Music
    sf::Music m_music;
    ToggleKey m_musicKey{sf::Keyboard::Key::M};
    bool m_musicStarted = false;

    // Extraction spawn tracking (non-static, reset per round)
    bool m_extractionSpawned = false;
};