#pragma once

#include <unordered_map>
#include <string>

#include <SDL2/SDL_keycode.h>

enum class PlayerControl
{
    MOVE_FORWARD,
    MOVE_BACK,
    STEER_LEFT,
    STEER_RIGHT,
    THROW_BOMB,
    SHOOT_LASER,
    BOOST
};

class KeyBindings
{

    std::unordered_map<SDL_KeyCode, std::string> m_key_names;

public:
    KeyBindings();

    bool setBinding(PlayerControl command, SDL_KeyCode new_key);
    bool commandNotSet(PlayerControl command);
    void unsetKey(SDL_KeyCode new_key);
    void unsetCommand(PlayerControl command);
    SDL_KeyCode operator[](PlayerControl command) const;
    std::string keyName(SDL_KeyCode key) const;
    std::string keyName(PlayerControl key) const;

private:
    std::unordered_map<PlayerControl, SDL_KeyCode> m_command_map;
    std::unordered_map<SDL_KeyCode, PlayerControl> m_key_map;
};

bool isKeyPressed(SDL_Keycode key);
// bool isButtonPressed(SDL_MOUSE_LEFT)