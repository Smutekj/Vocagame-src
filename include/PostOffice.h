#pragma once

#include <queue>
#include <unordered_map>
#include <tuple>
#include <memory>
#include <functional>
#include <typeindex>

#include "GameEvents.h"
#include "GameObject.h"

using SubscriptionId = int;

template <class MessageT>
using Callback = std::function<void(const std::deque<MessageT> &)>;
template <class T>
using SubscribersT = std::unordered_map<SubscriptionId, Callback<T>>;

class MessageHolderI
{
public:
    virtual ~MessageHolderI(){};
    virtual void distribute() = 0;
};

template <class MessageType>
class MessageHolder : public MessageHolderI
{
public:
    virtual ~MessageHolder() override{}

    void sendNow(MessageType message)
    {
        assert(false); //! TODO will implement
        
    }
    void send(MessageType message)
    {
        messages.push_back(message);
    }

    int subscribe(Callback<MessageType> subscriber)
    {
        int new_id = findNewFreeId();
        assert(subscribers.count(new_id) == 0);
        subscribers[new_id] = subscriber;
        return new_id;
    }

    void unsubscribe(int id)
    {
        subscribers.erase(id);
    }

    virtual void distribute() override
    {
        for (auto [id, call_back] : subscribers)
        {
            call_back(messages);
        }
        messages.clear();
    };

private:
    inline int findNewFreeId()
    {
        while (subscribers.count(next_id) != 0)
        {
            next_id++;
        }
        return next_id;
    }

    int next_id = 0;
    SubscribersT<MessageType> subscribers;
    std::deque<MessageType> messages;
};

struct CollisionEvent
{
    int id_a;
    int id_b;
};

// template <>
// class MessageHolder<CollisionEvent> : public MessageHolderI
// {

// public:
//     virtual void distribute()
//     {
//         for (auto event : messages)
//         {
            
//         }

//         for (auto [id, call_back] : type_type_subscribers)
//         {
//             call_back(messages);
//         }
//         messages.clear();
//     };

// private:
//     int next_id = 0;
//     SubscribersT<CollisionEvent> id_id_subscribers;
//     SubscribersT<CollisionEvent> id_type_subscribers;
//     SubscribersT<CollisionEvent> type_type_subscribers;

//     std::deque<CollisionEvent> messages;
// };


class PostOffice
{

public:
    ~PostOffice(){}
    
    void distributeMessages();

    template <class MessageDataT>
    void send(MessageDataT message);

    template <class MessageDataT>
    void registerEvent();

    template <class... MessageData>
    void registerEvents();

private:
    template <class MessageData>
    friend class PostBox;

    template <class MessageData>
    bool isRegistered() const;

    template <class MessageData>
    MessageHolder<MessageData> &getHolder();

    template <class MessageData>
    int subscribeTo(Callback<MessageData> &subscriber);

    template <class MessageDataT>
    void unsubscribe(int id);

    // template <class MessageType>
    // void distributeToSubscribers(std::deque<MessageType> &messages);

    std::unordered_map<std::type_index, std::unique_ptr<MessageHolderI>> m_holders;
};

// using PostOfficeA = PostOffice<EntityDiedEvent, EntityCreatedEvent, ObjectiveFinishedEvent>;

#include "PostOffice.inl"

// struct CollisionEvent2
// {
//     ObjectType type_a;
//     int id_a;
//     ObjectType type_b;
//     int id_b;
// };

// struct CollisionSubscriptionToken
// {
// };

// class CollisionEventPostOffice
// {

//     using Callback = std::function<void(CollisionEvent2 &)>;
//     template <class T>
//     using SubscribersT = std::unordered_map<int, Callback<T>>;

//     // using MessageTypes = std::tuple<std::deque<EntityDiedEvent>, std::deque<EntityCreatedEvent>, std::deque<ObjectiveFinishedEvent>>;
//     // using SubscriberTypes = std::tuple<SubscribersT<EntityDiedEvent>, SubscribersT<EntityCreatedEvent>, SubscribersT<ObjectiveFinishedEvent>>;

//     // using MessageTypes = std::tuple<std::deque<RegisteredTypes>...>;
//     // using SubscriberTypes = std::tuple<SubscribersT<RegisteredTypes>...>;

// public:
//     void distributeMessages()
//     {
//         for (auto &msg : m_to_send)
//         {
//             if (type_id2subscribers.contains({msg.type_a, msg.id_b}))
//             {for (auto &subscriber : type_id2subscribers.at({msg.type_a, msg.id_b}))
//             {
//                 subscriber(msg);
//             }
//         }
//             if (type_id2subscribers.contains({msg.type_b, msg.id_a}))
//             {
//             for (auto &subscriber : type_id2subscribers.at({msg.type_b, msg.id_a}))
//             {
//                 subscriber(msg);
//             }
//         }

//             if (type_type2subscribers.contains({msg.type_a, msg.type_b}))
//             {
//                 for (auto &subscriber : type_type2subscribers.at({msg.type_a, msg.type_b}))
//                 {
//                     subscriber(msg);
//                 }
//             }
//         }
//     }

//     template <class PairIdType>
//     void distributeToSubscribers(CollisionEvent2 message, PairIdType pair, std::unordered_map<PairIdType, )
//     {
//         if()
//         for (auto &subscriber : id_id2subscribers.at(id_pair))
//             {
//                 subscriber(message);
//             }
//     }

//     void send(CollisionEvent2 message)
//     {
//         std::pair<int, int> id_pair{message.id_a, message.id_b};
//         if (id_id2subscribers.contains(id_pair))
//         {

//         }
//         if (id_id2subscribers.contains({message.id_b, message.id_b}))
//         {

//         }

//         m_to_send.push_back(message);
//     }

//     template <class MessageDataT>
//     void registerEvent();

// private:
//     template <class MessageData>
//     friend class PostBox;

//     int subscribeTo(std::pair<int, int> id_pair, Callback &subscriber)
//     {
//         id_id2subscribers[id_pair].push_back(subscriber);
//     }
//     int subscribeTo(std::pair<ObjectType, int> type_id_pair, Callback &subscriber)
//     {
//         type_id2subscribers[type_id_pair].push_back(subscriber);
//     }
//     int subscribeTo(std::pair<ObjectType, ObjectType> type_pair, Callback &subscriber)
//     {
//         type_type2subscribers[type_pair].push_back(subscriber);
//     }

//     void unsubscribe(int id);

//     int findNewFreeId();

//     int next_id = 0;

//     std::unordered_map<std::pair<int, int>, std::vector<Callback>> id_id2subscribers;
//     std::unordered_map<std::pair<ObjectType, int>, std::vector<Callback>> type_id2subscribers;
//     std::unordered_map<std::pair<ObjectType, ObjectType>, std::vector<Callback>> type_type2subscribers;

//     std::deque<CollisionEvent2> m_to_send;
//     // SubscriberTypes m_subscribers;
// };

// using PostOfficeA = PostOffice<EntityDiedEvent, EntityCreatedEvent, ObjectiveFinishedEvent>;

// #include "PostOffice.inl"