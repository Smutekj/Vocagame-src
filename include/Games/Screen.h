#pragma once

#include "Renderer.h"

#include <SDL2/SDL_events.h>

//! this does not belong here but i don't care anymore
class Screen
{
public:
    virtual void update(float dt) = 0;
    virtual void handleEvent(const SDL_Event &event) = 0;
    virtual void draw(Renderer &window) = 0;
    virtual ~Screen() {};
};
