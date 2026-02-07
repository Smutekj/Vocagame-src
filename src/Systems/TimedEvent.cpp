#include "TimedEvent.h"

TimedEvent::TimedEvent(std::function<void(float, int)> callback, TimedEventType type, float period, float delay)
    : m_period(period), m_firing_time(period-delay), m_callback(callback), m_timer_type(type)

{
}

TimedEvent::TimedEvent(std::function<void(float, int)> callback,
                       float period, float delay, int repeats_count)
    : m_period(period), m_firing_time(period-delay), m_callback(callback),
      m_total_repeats_count(repeats_count), m_repeats_left(repeats_count), m_timer_type(TimedEventType::Fixed)
{
}

TimedEvent::TimedEvent(std::function<void(float, int)> callback,
                       float period)
    :TimedEvent(callback, period, period, 1)
{}

void TimedEvent::update(float dt)
{
    m_firing_time += dt;
    if (m_firing_time > m_period)
    {
        m_total_time += m_firing_time;
        if (m_callback && (m_repeats_left > 0 || m_timer_type == TimedEventType::Infinite))
        {
            int repeat_count = (m_total_repeats_count - m_repeats_left);
            m_callback(m_total_time, repeat_count);
            m_repeats_left--;
        }
        m_firing_time = 0.f;
    }
}

bool TimedEvent::isInfinite() const
{
    return m_timer_type == TimedEventType::Infinite;
}

float TimedEvent::getTimeLeft() const
{
    return m_firing_time + m_period * (m_repeats_left - 1);
}
int TimedEvent::getRepeatsLeft() const
{
    return m_repeats_left;
}
int TimedEvent::getTotalRepeats() const
{
    return m_total_repeats_count;
}