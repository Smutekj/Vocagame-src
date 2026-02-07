#pragma once

#include "System.h"
#include "../Components.h"
#include "../PostOffice.h"
#include "../PostBox.h"


class HealthSystem : public SystemI
{
public:
    HealthSystem(utils::ContiguousColony<HealthComponent, int> &comps, PostOffice& m_messenger);

    virtual void preUpdate(float dt, EntityRegistryT &entities) override;
    virtual void postUpdate(float dt, EntityRegistryT &entities) override;
    virtual void update(float dt) override;

private:
    utils::ContiguousColony<HealthComponent, int> &m_components;
    PostOffice* p_messenger = nullptr;    
    std::unique_ptr<PostBox<DamageReceivedEvent>> m_postbox;
};
