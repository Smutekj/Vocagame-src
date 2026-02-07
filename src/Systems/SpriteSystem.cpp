#include "SpriteSystem.h"

#include "Renderer.h"
#include "DrawLayer.h"
#include "Particles.h"

SpriteSystem::SpriteSystem(utils::ContiguousColony<SpriteComponent, int> &sprites, LayersHolder &layers)
    : m_components(sprites), m_layers(layers)
{
}

void SpriteSystem::preUpdate(float dt, EntityRegistryT &entities)
{
    auto &comps = m_components.data;
    auto &ids = m_components.data_ind2id;
    for (std::size_t comp_id = 0; comp_id < comps.size(); ++comp_id)
    {
        auto &comp = comps[comp_id];
        comp.sprite.setPosition(entities.at(ids[comp_id])->getPosition());
        comp.sprite.setRotation(utils::radians(entities.at(ids[comp_id])->getAngle()));
        comp.sprite.setScale(entities.at(ids[comp_id])->getSize() / 2.f);
    }
}
void SpriteSystem::postUpdate(float dt, EntityRegistryT &entities)
{
}
void SpriteSystem::update(float dt)
{
    auto &comps = m_components.data;
    auto &ids = m_components.data_ind2id;
    for (std::size_t comp_id = 0; comp_id < comps.size(); ++comp_id)
    {
        auto &comp = comps[comp_id];
        auto &canvas = m_layers.getCanvas(comp.layer_id);
        canvas.drawSprite(comp.sprite, comp.shader_id);
    }
}
/* 
ParticleSystem::ParticleSystem(utils::ContiguousColony<ParticleComponent, int> &comps, LayersHolder &layers)
    : m_components(comps), m_layers(layers)
{
}

void ParticleSystem::preUpdate(float dt, EntityRegistryT &entities)
{
    auto &comps = m_components.data;
    auto &ids = m_components.data_ind2id;
    for (std::size_t comp_id = 0; comp_id < comps.size(); ++comp_id)
    {
        auto &comp = comps[comp_id];
        // comp.sprite.setPosition(entities.at(ids[comp_id])->getPosition());
        // comp.sprite.setRotation(utils::radians(entities.at(ids[comp_id])->getAngle()));
        // comp.sprite.setScale(entities.at(ids[comp_id])->getSize()/2.f);
    }
}
void ParticleSystem::postUpdate(float dt, EntityRegistryT &entities)
{
    auto &comps = m_components.data;
    auto &ids = m_components.data_ind2id;
    for (std::size_t comp_id = 0; comp_id < comps.size(); ++comp_id)
    {
        auto &comp = comps[comp_id];
        auto &canvas = m_layers.getCanvas(comp.layer_id);
        comp.particles->draw(canvas);
    }
}
void ParticleSystem::update(float dt)
{
    auto &comps = m_components.data;
    auto &ids = m_components.data_ind2id;
    for (std::size_t comp_id = 0; comp_id < comps.size(); ++comp_id)
    {
        auto &comp = comps[comp_id];
        comp.particles->update(dt);
    }
} */

TransformSystem::TransformSystem(utils::ContiguousColony<TransformComponent, int> &sprites)
    : m_components(sprites)
{
}

void TransformSystem::preUpdate(float dt, EntityRegistryT &entities)
{
    std::vector<int> to_destroy;

    auto &comps = m_components.data;
    auto &ids = m_components.data_ind2id;
    for (std::size_t comp_id = 0; comp_id < comps.size(); ++comp_id)
    {
        auto &comp = comps[comp_id];

        auto &entity = entities.at(ids[comp_id]);
        auto pos = entity->getPosition();
        float dist2 = utils::norm2(comp.target_pos - pos);
        if(comp.duration > 0.f)
        {
            entity->setPosition(pos + dt / comp.duration * (comp.target_pos - pos));
            comp.duration -= dt;
        }else{
            to_destroy.push_back(ids[comp_id]);
        }
        // comp.sprite.setRotation(utils::radians(entities.at(ids[comp_id])->getAngle()));
        // comp.sprite.setScale(entities.at(ids[comp_id])->getSize()/2.f);
    }

    while(!to_destroy.empty())
    {
        m_components.erase(to_destroy.back());
        to_destroy.pop_back();
    }
}
void TransformSystem::update(float dt) {}

void TransformSystem::postUpdate(float dt, EntityRegistryT &entities)
{
}