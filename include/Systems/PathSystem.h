#pragma once

#include "System.h"
#include "Components.h"

class LayersHolder;
class Path;

class PathSystem : public SystemI
{
public:
    PathSystem(utils::ContiguousColony<PathComponent, int> &boids, LayersHolder& layers);

    virtual void preUpdate(float dt, EntityRegistryT &entities) override;
    virtual void postUpdate(float dt, EntityRegistryT &entities) override;
    virtual void update(float dt) override;

private:
    //! I could also store all of the sprite data directly in PathBatch to avoid copying
    //! But it's not a bottleneck right now so who cares?
    utils::ContiguousColony<PathComponent, int> &m_components;
    LayersHolder& m_layers;
};
