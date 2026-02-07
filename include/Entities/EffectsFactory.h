#pragma once

#include "FactoryBase.h"
#include "ObjectRegistry.h"

#include <functional>
#include <unordered_map>
#include <Utils/Vector2.h>

enum class EffectId
{
    StarBurst,
    Fireworks1,
    BlueExplosion,
    PurpleExplosion,
};

class GameWorld;

struct EffectSpec
{
    utils::Vector2f pos;
    utils::Vector2f size;
    float life_time;
};
class EffectsFactory
{

public:
    EffectsFactory(GameWorld &world);

    void create(EffectId id, utils::Vector2f pos, utils::Vector2f size, float life_time);

private:
    void registerFactories();

private:
    GameWorld &m_world;
    std::unordered_map<ObjectType, std::shared_ptr<FactoryBase>> m_factories;
    std::unordered_map<EffectId, std::function<void(EffectSpec)>> m_makers;
};
