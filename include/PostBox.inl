
#pragma once

#include "PostOffice.h"

template <class MessageData>
PostBox<MessageData>::PostBox(PostOffice &post_office,
                              std::function<void(const std::deque<MessageData> &)> on_receival)
    : p_post_office(&post_office), on_receival(on_receival)
{
    id = p_post_office->subscribeTo<MessageData>(on_receival);
}

template <class MessageData>
PostBox<MessageData>::~PostBox()
{
    p_post_office->unsubscribe<MessageData>(id);
}

template <class MessageData>
PostBox<MessageData>::PostBox(const PostBox& other)
: p_post_office(other.p_post_office), on_receival(other.on_receival)
{
    id = p_post_office->subscribeTo(on_receival);
}

template <class MessageData>
PostBox<MessageData>::PostBox(PostBox&& other)
: p_post_office(std::move(other.p_post_office)), on_receival(std::move(other.on_receival))
{
    p_post_office->unsubscribe<MessageData>(other.id);
    id = p_post_office->subscribeTo(on_receival);
}