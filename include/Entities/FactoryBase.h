#pragma once

#include <memory>

class GameObject;
class GameObjectSpec;

class FactoryBase
{
public:
    virtual ~FactoryBase() = default;

    GameObject &create(const GameObjectSpec &spec);

    virtual std::shared_ptr<GameObjectSpec> makeSpec() = 0;
private:
    virtual GameObject &createImpl(const GameObjectSpec &spec) = 0;
};