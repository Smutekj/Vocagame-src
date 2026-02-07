#include "ObjectArena.h"

/* ObjectArena::ObjectArena()
{
    m_chunk_list.emplace_back(CHUNK_SIZE);
}

void ObjectArena::registerObject(int type_id, std::size_t obj_size)
{
    if (m_obj_sizes.count(type_id) > 0 && m_obj_sizes.at(type_id) != obj_size)
    {
        std::cout << "WARNING OBJECT ALREADY REGISTERED AND HAS DIFFERENT SIZE!" << std::endl;
        return;
    }
    m_obj_sizes[type_id] = obj_size;
    m_freelists[type_id] = nullptr;
}

void ObjectArena::deallocateObject(int type_id, void *obj)
{
    std::size_t obj_size = m_obj_sizes.at(type_id);

    std::byte *curr_head = m_freelists.at(type_id);
    m_freelists.at(type_id) = static_cast<std::byte *>(obj);
    *(static_cast<std::byte **>(obj)) = curr_head;
};

void *ObjectArena::allocateObject(int type_id)
{
    if (m_freelists.count(type_id) == 0)
    {
        return nullptr;
    }

    std::byte *list_head = m_freelists.at(type_id);
    std::size_t obj_size = m_obj_sizes.at(type_id);

    if (!list_head) //! new free spot will be placed to the right
    {
        if (m_max_allocated_size + obj_size < m_chunk_list.back().size())
        {
            m_max_allocated_size += obj_size;
            return (m_chunk_list.back().data() + (m_max_allocated_size - obj_size));
        }
        else //! new obj does not fit inside last chunk -> allocate new chunk
        {
            std::size_t prev_chunk_size = m_chunk_list.back().size();
            m_chunk_list.emplace_back(prev_chunk_size * 2); //! increase size by a factor
            m_max_allocated_size = obj_size;
            return m_chunk_list.back().data();
        }
    }

    auto next_head = *(reinterpret_cast<std::byte **>(list_head));
    m_freelists.at(type_id) = next_head;

    return list_head;
} */