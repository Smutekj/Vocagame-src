#pragma once

#include <Utils/ObjectPool.h>
#include "TimedEvent.h"

using TimedEventId = int;


class TimedEventManager
{
public:
    TimedEventManager();

    TimedEventId addInfiniteEvent(std::function<void(float, int)> callback, float period, float delay = 0.f);
    TimedEventId addTimedEvent(std::function<void(float, int)> callback, float period, float delay, int repeats_count = 1);
    TimedEventId addTimedEvent(std::function<void(float, int)> callback, float period);
    std::vector<TimedEventId> addEventSeries(std::function<void(float, int)> callback, std::vector<float> delays, int repeats_count = 1);
    TimedEventId addTimedEvent(TimedEvent& event);
    TimedEventId runDelayed(std::function<void()> callback, float delay);


    void removeEvent(TimedEventId id);
    void clear();
    void update(float dt);

private:
    std::vector<TimedEventId> m_to_destroy;
    utils::VectorMap<TimedEvent> m_events;
};