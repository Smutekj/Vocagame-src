
#include "Utils/Vector2.h"
#include "Factories.h"
#include "../PostOffice.h"
#include "../PostBox.h"

#include <functional>


class SpawnerI
{
public:
    SpawnerI(GameWorld &world, int max_alive_count = -1);

    virtual ~SpawnerI();

    void setMaxCount(int max_count);

    void update(float dt)
    {
        m_events.update(dt);
    }

public:
    bool m_kill_on_destriuction = true;

protected:
    int m_max_alive_count = -1;
    std::unique_ptr<PostBox<EntityDiedEvent>> m_on_death;
    TimedEventManager m_events;
    std::unordered_set<int> m_spawned_ids;

    GameWorld &m_world;
};

template <class FactoryType>
class SpawnerT : public SpawnerI
{
public:
    SpawnerT(GameWorld &world, TextureHolder &textures, typename FactoryType::Spec spec,
             std::function<void(typename FactoryType::Spec &, float, int)> specifier,
             float interval, float delay = 0.f, int spawn_count = -1);
    SpawnerT(GameWorld &world, TextureHolder &textures, FactoryType::Spec spec,
             std::function<void(typename FactoryType::Spec &, float, int)> specifier, std::function<void(float, int)> delay_generator);

    virtual ~SpawnerT() = default;

    void periodicSpawn(float t, int c)
    {
        TimedEvent new_event = {m_delay_generator(t, c), [this](float t, int c)
                                {
                                    spawn();
                                    periodicSpawn(t, c);
                                },
                                1, m_delay_generator(t, c)};
        m_events.addTimedEvent(new_event);
    }

    void spawn()
    {
        if (m_max_alive_count == -1 || m_spawned_ids.size() < m_max_alive_count)
        {
            m_specifier(m_spec);
            auto &obj = m_factory.create(m_spec);
            m_spawned_ids.insert(obj.getid());
        }
    }

private:
    std::function<void(float, int)> m_delay_generator;
    std::function<void(typename FactoryType::Spec &, float, int)> m_specifier;
    FactoryType m_factory;
    typename FactoryType::Spec m_spec;
};

template <class FactoryType>
SpawnerT<FactoryType>::SpawnerT(GameWorld &world, TextureHolder &textures, typename FactoryType::Spec spec,
                                std::function<void(typename FactoryType::Spec &, float, int)> specifier,
                                float interval, float delay, int spawn_count)
    : SpawnerI(world),
      m_spec(spec), m_factory(world, textures), m_specifier(specifier)
{
    auto spawn_event = [this](float t, int c)
    {
        m_specifier(m_spec, t, c);
        auto &obj = m_factory.create(m_spec);
        m_spawned_ids.insert(obj.getId()); };

    m_events.addTimedEvent(spawn_event, interval, delay, spawn_count);
}
template <class FactoryType>
SpawnerT<FactoryType>::SpawnerT(GameWorld &world, TextureHolder &textures, typename FactoryType::Spec spec,
                                std::function<void(typename FactoryType::Spec &, float, int)> specifier, std::function<void(float, int)> delay_generator)
    : SpawnerI(world),
      m_spec(spec), m_specifier(specifier), m_factory(world, textures), m_delay_generator(delay_generator)
{
    periodicSpawn(0, 0);
}
