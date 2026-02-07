#pragma once

#include <functional>

enum class TimedEventType
{
    Fixed,
    Infinite
};

struct TimedEvent
{
    TimedEvent(std::function<void(float, int)> callback, TimedEventType type, float period, float delay = 0.f);
    TimedEvent(std::function<void(float, int)> callback, float period, float delay, int repeats_count = 1);
    TimedEvent(std::function<void(float, int)> callback, float period);

    void update(float dt);

    bool isInfinite() const;

    float getTimeLeft() const;
    int getRepeatsLeft() const;
    int getTotalRepeats() const;

private:
    TimedEventType m_timer_type;

    float m_period;
    float m_firing_time;
    float m_total_time = 0.f;

    int m_repeats_left = 1;
    int m_total_repeats_count = 1;

    std::function<void(float, int)> m_callback = [](float, int) {};
};
