#pragma once

inline void PostOffice::distributeMessages()
{
    for (auto &[type_id, holder] : m_holders)
    {
        holder->distribute();
    }
}

template <class MessageDataT>
inline void PostOffice::send(MessageDataT message)
{
    getHolder<MessageDataT>().send(message);
}

template <class MessageDataT>
inline int PostOffice::subscribeTo(Callback<MessageDataT> &subscriber)
{
    auto &holder = getHolder<MessageDataT>();
    return holder.subscribe(subscriber);
}

template <class MessageDataT>
bool PostOffice::isRegistered() const
{
    return m_holders.count(std::type_index(typeid(MessageDataT))) > 0;
}

template <class MessageDataT>
MessageHolder<MessageDataT> &PostOffice::getHolder()
{
    auto type_id = std::type_index(typeid(MessageDataT));
    if (!m_holders.contains(type_id))
    {
        m_holders[type_id] = std::make_unique<MessageHolder<MessageDataT>>();
    }
    return static_cast<MessageHolder<MessageDataT> &>(*m_holders.at(std::type_index(typeid(MessageDataT))));
}
template <class MessageDataT>
inline void PostOffice::unsubscribe(int id)
{
    getHolder<MessageDataT>().unsubscribe(id);
}

template <class MessageDataT>
void PostOffice::registerEvent()
{
    m_holders[std::type_index(typeid(MessageDataT))] = std::make_unique<MessageHolder<MessageDataT>>();
}

template <class... MessageDataT>
void PostOffice::registerEvents()
{
    (registerEvent<MessageDataT>(), ...);
}