#pragma once

#include <vector>
#include <array>
#include <unordered_map>
#include <list>
#include <memory>
#include <cstddef>
#include <cassert>
// #include <iostream>

constexpr std::size_t CHUNK_SIZE = 1 << 14;
class ObjectArena
{
public:
    ObjectArena()
    {
        m_chunk_list.emplace_back(); // CHUNK_SIZE);
    }

    void registerObject(int type_id, std::size_t obj_size)
    {
        if (m_obj_sizes.count(type_id) > 0 && m_obj_sizes.at(type_id) != obj_size)
        {
            // std::cout << "WARNING OBJECT ALREADY REGISTERED AND HAS DIFFERENT SIZE!" << std::endl;
            return;
        }
        m_obj_sizes[type_id] = obj_size;
        m_freelists[type_id] = nullptr;
    }

    void deallocateObject(int type_id, void *obj)
    {
        std::size_t obj_size = m_obj_sizes.at(type_id);

        std::byte *curr_head = m_freelists.at(type_id);
        m_freelists.at(type_id) = static_cast<std::byte *>(obj);
        *(static_cast<std::byte **>(obj)) = curr_head;
    };

    void *allocateObject(int type_id)
    {
        if (m_freelists.count(type_id) == 0)
        {
            return nullptr;
        }

        std::byte *list_head = m_freelists.at(type_id);
        std::size_t obj_size = m_obj_sizes.at(type_id);

        // std::cout << "Creating new object a!" << std::endl;
        if (!list_head) //! new free spot will be placed to the right
        {
            // std::cout << "Creating new object!" << std::endl;
            if (m_max_allocated_size + obj_size < m_chunk_list.back().chunk.size())
            {
                m_max_allocated_size += obj_size;
                return (m_chunk_list.back().chunk.data() + (m_max_allocated_size - obj_size));
            }
            else //! new obj does not fit inside last chunk -> allocate new chunk
            {
                // std::size_t prev_chunk_size = m_chunk_list.back().chunk.size();
                m_chunk_list.emplace_back(); //! increase size by a factor
                m_max_allocated_size = obj_size;
                return m_chunk_list.back().chunk.data();
            }
        }

        auto next_head = *(reinterpret_cast<std::byte **>(list_head));
        m_freelists.at(type_id) = next_head;
        return list_head;
    }

private:
    std::size_t m_max_allocated_size = 0;

    std::unordered_map<int, std::byte *> m_freelists;
    std::unordered_map<int, std::size_t> m_obj_sizes;

    struct alignas(32) Chunk
    {
        std::array<std::byte, CHUNK_SIZE> chunk;
    };
    std::list<Chunk> m_chunk_list;
    
};
