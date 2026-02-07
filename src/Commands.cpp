#include "Commands.h"

#include <cassert>

#include <SDL2/SDL_keyboard.h>

KeyBindings::KeyBindings()
{
    m_command_map[PlayerControl::MOVE_FORWARD] = SDLK_w;
    m_command_map[PlayerControl::MOVE_BACK] = SDLK_s;
    m_command_map[PlayerControl::STEER_LEFT] = SDLK_a;
    m_command_map[PlayerControl::STEER_RIGHT] = SDLK_d;
    m_command_map[PlayerControl::THROW_BOMB] = SDLK_b;
    m_command_map[PlayerControl::SHOOT_LASER] = SDLK_SPACE;
    m_command_map[PlayerControl::BOOST] = SDLK_LSHIFT;

    for (auto [command, key] : m_command_map)
    {
        m_key_map[key] = command;
    }

    m_key_names[SDL_KeyCode::SDLK_LSHIFT] = "Left Shift";
    m_key_names[SDL_KeyCode::SDLK_RSHIFT] = "Left Shift";
    m_key_names[SDL_KeyCode::SDLK_LCTRL] = "Left Ctrl";
    m_key_names[SDL_KeyCode::SDLK_RCTRL] = "Right Ctrl";
    m_key_names[SDL_KeyCode::SDLK_LALT] = "Left Alt";
    m_key_names[SDL_KeyCode::SDLK_RALT] = "Right Alt";
    m_key_names[SDL_KeyCode::SDLK_SPACE] = "Space";
    m_key_names[SDL_KeyCode::SDLK_TAB] = "Tab";
    
    m_key_names[SDL_KeyCode::SDLK_LEFT] = "Left Arrow";
    m_key_names[SDL_KeyCode::SDLK_RIGHT] = "Right Arrow";
    m_key_names[SDL_KeyCode::SDLK_UP] = "Up Arrow";
    m_key_names[SDL_KeyCode::SDLK_DOWN] = "Down Arrow";

    for (int key_ind = SDL_KeyCode::SDLK_0; key_ind <= SDL_KeyCode::SDLK_9; key_ind += 1)
    {
        m_key_names[(SDL_KeyCode)key_ind] = (char)key_ind;
    }

    for (int key_ind = SDL_KeyCode::SDLK_a; key_ind <= SDL_KeyCode::SDLK_z; key_ind += 1)
    {
        m_key_names[(SDL_KeyCode)key_ind] = (char)key_ind;
    }
}

bool KeyBindings::setBinding(PlayerControl command, SDL_KeyCode new_key)
{
    auto old_key = m_command_map.at(command);
    m_key_map.erase(old_key);

    if (m_key_map.count(new_key) > 0) //! if key is already bound we switch the commands
    {
        auto old_command = m_key_map.at(new_key);
        m_command_map[old_command] = old_key;
        m_key_map[old_key] = old_command;
    }

    m_command_map[command] = new_key;
    m_key_map[new_key] = command;

    return true;
}

std::string KeyBindings::keyName(SDL_KeyCode key) const
{
    if (m_key_names.count(key) == 0)
    {
        return "Unknown key name";
    }
    return m_key_names.at(key);
}

std::string KeyBindings::keyName(PlayerControl command) const
{
    return keyName(m_command_map.at(command));
}

bool KeyBindings::commandNotSet(PlayerControl command)
{
    return m_command_map.count(command) == 0;
}

void KeyBindings::unsetKey(SDL_KeyCode new_key)
{
    assert(m_key_map.count(new_key) > 0);
    m_key_map.erase(new_key);
}
void KeyBindings::unsetCommand(PlayerControl command)
{
    auto old_key = m_command_map.at(command);
    m_key_map.erase(old_key);
}

SDL_KeyCode KeyBindings::operator[](PlayerControl command) const
{
    return m_command_map.at(command);
}

bool isKeyPressed(SDL_Keycode key)
{
    auto *keystate = SDL_GetKeyboardState(NULL);
    return keystate[SDL_GetScancodeFromKey(key)];
}