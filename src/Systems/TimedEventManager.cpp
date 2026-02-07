#include "TimedEventManager.h"
#include <numeric>

constexpr int MAX_TIMED_EVENT_COUNT = 1000;

TimedEventManager::TimedEventManager()
    : m_events(MAX_TIMED_EVENT_COUNT) {}

TimedEventId TimedEventManager::addInfiniteEvent(
    std::function<void(float, int)> callback,
    float period,
    float delay)
{
    TimedEvent event = {callback, TimedEventType::Infinite, period, delay};
    return m_events.insert(event);
}

std::vector<TimedEventId> TimedEventManager::addEventSeries(
    std::function<void(float, int)> callback,
    std::vector<float> delays,
    int repeats_count)
{
    std::vector<TimedEventId> ids;
    if (delays.size() == 0)
    {
        return ids;
    }

    float series_period = std::accumulate(delays.begin(), delays.end(), 0.f);

    float firing_time = 0.f;
    for (auto delay : delays)
    {
        firing_time += delay;
        TimedEvent event = {callback, series_period, firing_time, repeats_count};
        auto new_id = m_events.insert(event);
        ids.push_back(new_id);
    }

    return ids;
}

TimedEventId TimedEventManager::addTimedEvent(TimedEvent &event)
{
    return m_events.insert(event);
}

TimedEventId TimedEventManager::addTimedEvent(
    std::function<void(float, int)> callback,
    float period)
{
    return addTimedEvent(callback, period, 0.f, 1);
}

TimedEventId TimedEventManager::addTimedEvent(
    std::function<void(float, int)> callback,
    float period,
    float delay,
    int repeats_count)
{
    if (repeats_count == -1)
    {
        return addInfiniteEvent(callback, period, delay);
    }

    TimedEvent event = {callback, period, delay, repeats_count};
    return m_events.insert(event);
}

void TimedEventManager::removeEvent(TimedEventId id)
{
    m_to_destroy.push_back(id);
}

void TimedEventManager::update(float dt)
{
    auto &events = m_events.getData();
    for (std::size_t i = 0; i < events.size(); ++i)
    {
        auto &event = events[i];
        event.update(dt);
        if (event.getRepeatsLeft() == 0 && !event.isInfinite())
        {
            m_to_destroy.push_back(m_events.getEntityInd(i));
        }
    }

    while (!m_to_destroy.empty())
    {
        m_events.removeByEntityInd(m_to_destroy.back());
        m_to_destroy.pop_back();
    }
}
TimedEventId TimedEventManager::runDelayed(std::function<void()> callback, float delay)
{
    return addTimedEvent([callback](float t, int c)
                         { callback(); }, 0.f, delay);
}
void TimedEventManager::clear()
{
    m_events.clear();
}