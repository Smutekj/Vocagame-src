#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

namespace utils
{

    template <class DataType, class IdType>
    struct ContiguousColony
    {
        ContiguousColony()
        {
            //! TODO: DO NOT BE AN IDIOT AND FIX THIS! THE MEMORY SHOULD BE STATICALLY ALLOCATED ANYWAY
            data.reserve(5000);
            data_ind2id.reserve(5000);
        }

        void clear()
        {
            data.clear();
            data_ind2id.clear();
            id2data_ind.clear();
        }

        void reserve(std::size_t new_size)
        {
            data.reserve(new_size);
            data_ind2id.reserve(new_size);
        }

        void insert(IdType id, auto &&datum)
        {
            data.emplace_back(std::forward<decltype(datum)>(datum));
            data_ind2id.push_back(id);

            assert(id2data_ind.count(id) == 0);
            id2data_ind[id] = data.size() - 1;
        }

        DataType &get(IdType id)
        {
            return data.at(id2data_ind.at(id));
        }

        void erase(IdType id)
        {
            assert(id2data_ind.count(id) != 0);
            std::size_t data_ind = id2data_ind.at(id);

            IdType swapped_id = data_ind2id.at(data.size() - 1);
            id2data_ind.at(swapped_id) = data_ind; //! swapped points to erased

            data.at(data_ind) = std::move(data.back());    //! swap
            data.pop_back();                               //! and pop
            data_ind2id.at(data_ind) = data_ind2id.back(); //! swap
            data_ind2id.pop_back();                        //! and pop

            id2data_ind.erase(id);
        }

        bool isEmpty() const
        {
            return data.empty();
        }

        void checkConsistency() const
        {
            for (auto [entity_id, data_id] : id2data_ind)
            {
                assert(data_ind2id.at(data_id) == entity_id);
            }
            for (int data_id = 0; data_id < data_ind2id.size(); ++data_id)
            {
                assert(data_id == id2data_ind.at(data_ind2id.at(data_id)));
            }

            //! entity ids should be a permutation of data ids
            std::unordered_set<int> entity_inds;
            int max_entity_id = -1;
            entity_inds.insert(data_ind2id.begin(), data_ind2id.end());
            for (int data_id = 0; data_id < data_ind2id.size(); ++data_id)
            {
                max_entity_id = std::max(data_ind2id.at(data_id), max_entity_id);
                entity_inds.erase(data_id);
                // m_data_inds.insert(data_id);
            }
            // assert(max_entity_id < data.size());
        }

        std::size_t size() const
        {
            return data.size();
        }

        bool contains(IdType id) const
        {
            return id2data_ind.contains(id);
        }

    public:
        std::vector<DataType> data;
        std::vector<IdType> data_ind2id;

    private:
        std::unordered_map<IdType, std::size_t> id2data_ind;
    };

    template <typename DataType>
    class DynamicObjectPool2
    {
    public:
        int insert(auto &&data)
        {
            int id = reserveIndexForInsertion();
            m_data.insert(id, data);
            return id;
        }

        bool contains(int entity_id) const
        {
            return m_data.contains(entity_id);
        }

        int reserveIndexForInsertion()
        {
            int id = m_next_id;
            if (!m_free_list.empty())
            {
                id = m_free_list.back();
                m_free_list.pop_back();
            }

            m_next_id++;
            return id;
        }

        void insertAt(int index, auto &&datum)
        {
            assert(!m_data.contains(index));
            m_data.insert(index, datum);
        }

        std::vector<DataType> &data()
        {
            return m_data.data;
        }
        std::vector<int> &getIds()
        {
            return m_data.data_ind2id;
        }

        DataType &at(int index)
        {
            return m_data.get(index);
        }

        void remove(int id)
        {
            m_data.erase(id);
            m_free_list.push_back(id);
            m_next_id--;
        }

    private:
        int m_next_id = 0;
        utils::ContiguousColony<DataType, int> m_data;
        std::vector<int> m_free_list;
    };

} //! namespace utils