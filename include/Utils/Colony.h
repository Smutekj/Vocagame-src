#pragma once

#include <functional>
#include <array>
// #include <cstdint>
#include <variant>

template <class ComponentType>
class ComponentBlockI
{
public:
    virtual void update(const std::function<void(ComponentType&)> &) = 0;
    virtual int insert(ComponentType comp) = 0;
    virtual void deactivate(int ind) = 0;
};

template <class ComponentType, int BLOCK_CAPACITY = 3000>
class ComponentBlock : public ComponentBlockI<ComponentType>
{
public:
    ComponentBlock()
    {
        block.at(FIRST_ACTIVE).next = 1;
        block.at(ERASED_GROUPS_HEAD).next = ERASED_GROUPS_HEAD;
        block.at(ERASED_GROUPS_HEAD).erased.size = 0;
        
        block.at(0).next = BLOCK_CAPACITY+1;

        states.at(0) = State::Inactive;
        states.front() = State::Active;
        states.back() = State::Active;
    }

    struct ErasedGroup
    {
        int prev = 0;
        int size = 0;
    };

    struct Data
    {
        // constexpr Data() noexcept : comp(), next(1) {}
        // constexpr ~Data() noexcept {}

        // std::variant<ComponentType, ErasedGroup> comp;
        ComponentType comp;
        ErasedGroup erased;

        int next = 1;
    };

    enum class State : int
    {
        Inactive = 0,
        Active = 1,
        ToBeActivated = 2,
    };

    std::array<Data, BLOCK_CAPACITY + 2> block;
    std::array<State, BLOCK_CAPACITY + 2> states;

    const int ERASED_GROUPS_HEAD = BLOCK_CAPACITY + 1;
    const int FIRST_ACTIVE = 0;

    int last_ind = 0;
    int active_count = 0;
    int erased_group_count = 0;

    ComponentType& get(int id)
    {
        assert(states.at(id)==State::Active);
        return block.at(id).comp;
    } 

    //! returns number of free spots
    std::size_t freeCount() const
    {
        return BLOCK_CAPACITY - active_count;
    }
    std::size_t activeCount() const
    {
        return active_count;
    }

    int insert(ComponentType comp)
    {
        if (freeCount() <= 0)
        {
            return -1;
        }
        active_count++;

        if (erased_group_count == 0) //! if there are not erased groups, insert to end of data block
        {
            int ind = active_count;
            block.at(ind).comp = comp;
            block.at(ind).next = (ERASED_GROUPS_HEAD - ind);
            block.at(ind - 1).next = 1;

            states.at(ind) = State::Active;

            last_ind++;
            return ind;
        }

        int free_group_head_ind = block.at(ERASED_GROUPS_HEAD).next;
        int free_group_next = block.at(free_group_head_ind).next;
        int free_group_head_size = block.at(free_group_head_ind).erased.size;

        //! we always take the right element of a free group, because then there is no need for updating next in the free list
        int ind = free_group_head_ind + free_group_head_size - 1;

        if (free_group_head_size == 1) //! will remove the group from the free-list
        {
            removeFromFreeList(free_group_head_ind);
        }
        else
        {
            //! update free_group_size
            block.at(free_group_head_ind + free_group_head_size - 2).erased.size = free_group_head_size - 1;
            block.at(free_group_head_ind).erased.size = free_group_head_size - 1;
        }

        int prev_active_ind = free_group_head_ind - 1;
        block.at(prev_active_ind).next = free_group_head_size;
        block.at(ind).next = 1;

        states.at(ind) = State::Active;
        block.at(ind).comp = comp;

        return ind;
    }

    void deactivate(int ind)
    {
        active_count--;

        State left_state = states.at(ind - 1);
        State right_state = states.at(ind + 1);

        if (ind == last_ind)
        {
            last_ind--;
            int prev_active_ind = ind - 1;
            block.at(ind).next = ERASED_GROUPS_HEAD;
            if (left_state == State::Inactive) //! there is an erased block to the left, so we disable it as well
            {
                last_ind -= block.at(ind - 1).erased.size;
                int free_group = ind - block.at(ind - 1).erased.size;
                removeFromFreeList(free_group);
                prev_active_ind = free_group - 1;
            }
            block.at(prev_active_ind).next = (ERASED_GROUPS_HEAD - prev_active_ind); //! left active skips to the end

            return;
        }

        if (left_state == State::Active && right_state == State::Active) //! create new group
        {
            //! update skip field
            block.at(ind - 1).next += 1;
            //! update free list
            addToFreeListHead(ind);
            //! update size
            block.at(ind).erased.size = 1;
        }
        else if (left_state == State::Inactive && right_state == State::Active) //! add to the right of group
        {
            //! update skip field
            int left_active_ind = ind - 1 - block.at(ind - 1).erased.size;
            block.at(left_active_ind).next += 1;
            //! update free list
            int old_group_size = block.at(ind - 1).erased.size;
            int free_group_ind = ind - old_group_size;
            //! update group size
            block.at(free_group_ind).erased.size = old_group_size + 1;
            block.at(ind).erased.size = old_group_size + 1;
        }
        else if (left_state == State::Active && right_state == State::Inactive) //! add to the left of group
        {
            //! update skip field --> left inherits my skip + 1
            int left_active_ind = ind - 1;
            block.at(left_active_ind).next = 1 + block.at(ind).next;

            //! update free list
            int old_free_group_ind = ind + 1;
            int old_group_size = block.at(old_free_group_ind).erased.size;
            //! tell free list group_ind has changed
            int next_free_group = block.at(old_free_group_ind).next;
            int prev_free_group = block.at(old_free_group_ind).erased.prev;
            block.at(prev_free_group).next = ind;
            block.at(next_free_group).erased.prev = ind;
            //! update group size by inheriting old free group pointers and incrementing size
            block.at(ind).next = block.at(old_free_group_ind).next;
            block.at(ind).erased = block.at(old_free_group_ind).erased;
            block.at(ind).erased.size += 1;
            block.at(ind + old_group_size).erased.size = old_group_size + 1;
        }
        else if (left_state == State::Inactive && right_state == State::Inactive) //! merge two groups
        {
            //! update skip field;
            int left_group_size = block.at(ind - 1).erased.size;
            int right_group_size = block.at(ind + 1).erased.size;

            int left_active_ind = ind - 1 - left_group_size;
            block.at(left_active_ind).next += 1 + right_group_size;

            //! udpate free list
            int right_group_ind = ind + 1;
            int left_group_ind_l = ind - block.at(ind - 1).erased.size;
            int left_group_ind_r = ind + block.at(ind + 1).erased.size;
            //! update group size;
            block.at(left_group_ind_l).erased.size = 1 + left_group_size + right_group_size;
            block.at(left_group_ind_r).erased.size = 1 + left_group_size + right_group_size;

            removeFromFreeList(right_group_ind);
        }
        else
        {
            throw std::runtime_error("SHOULD NOT GET HERE!");
        }

        states.at(ind) = State::Inactive;
    }

    auto& getData()
    {
        return block;
    }

    void update(const std::function<void(ComponentType&)> &updater)
    {
        int first_active = block.at(FIRST_ACTIVE).next;
        for (int ind = first_active; ind <= BLOCK_CAPACITY; ind += block[ind].next)//block[ind].next)
        {
            updater(block[ind].comp);
        }

    }
    double sum = 0.;

private:
    void removeFromFreeList(int group_ind)
    {
        erased_group_count--;

        int prev_group_ind = block.at(group_ind).erased.prev;
        int next_group_ind = block.at(group_ind).next;

        block.at(prev_group_ind).next = next_group_ind;
        block.at(next_group_ind).erased.prev = prev_group_ind;
    }

    void addToFreeListHead(int group_ind)
    {
        erased_group_count++;

        int head_group_ind = block.at(ERASED_GROUPS_HEAD).next;
        block.at(head_group_ind).erased.prev = group_ind;

        block.at(group_ind).next = head_group_ind;
        block.at(group_ind).erased.prev = ERASED_GROUPS_HEAD;

        block.at(ERASED_GROUPS_HEAD).next = group_ind;
    }
};

// template <class ComponentType, int BLOCK_SIZE = 100>
// class Colony
// {

// public:
//     Colony()
//     {
//         m_block_list.push_back({std::make_shared<ComponentBlock<ComponentType, BLOCK_SIZE>>(),
//                                 -1,
//                                 -1,
//                                 BLOCK_SIZE});
//         free_spots_count += BLOCK_SIZE;
//         block_head = 0;
//     }

//     std::pair<int, int> insert(ComponentType comp)
//     {
//         int free_block_ind = block_head;
//         Block block_data = m_block_list.at(free_block_ind);

//         int internal_ind = block_data.p_block->insert(comp);

//         free_spots_count -= 1;
//         m_block_list.at(free_block_ind).free_count -= 1;
//         if (free_spots_count == 0)
//         {
//             addNewBlock();
//             free_spots_count += BLOCK_SIZE;
//         }
//         if (m_block_list.at(free_block_ind).free_count == 0) //! remove full block
//         {
//             removeBlock(free_block_ind);
//         }

//         return {free_block_ind, internal_ind};
//     }

//     void deactivate(std::pair<int, int> id)
//     {
//         Block &block_data = m_block_list.at(id.first);
//         block_data.p_block->deactivate(id.second);
//         block_data.free_count += 1;
//         free_spots_count += 1;
//         if (block_data.free_count == 1)
//         {
//             block_data.prev = -1;
//             block_data.next = block_head;
//             m_block_list.at(block_head).prev = id.first;
//             block_head = id.first;
//         }
//     }

//     void update(const std::function<void(ComponentType&)> &updater)
//     {
//         for (auto &b : m_block_list)
//         {
//             b.p_block->update(updater);
//         }
//         auto toc = std::chrono::high_resolution_clock::now();
//     }

// private:
//     void addNewBlock()
//     {
//         //! last added is the new head
//         int new_head = m_block_list.size();
//         m_block_list.push_back({std::make_shared<ComponentBlock<ComponentType, BLOCK_SIZE>>(), block_head, -1, BLOCK_SIZE});

//         m_block_list.at(block_head).prev = new_head;
//         block_head = new_head;
//     }

//     void removeBlock(int block_ind)
//     {
//         assert(m_block_list.at(block_ind).free_count == 0); //! remove only empty blocks;
//         int prev = m_block_list.at(block_ind).prev;
//         int next = m_block_list.at(block_ind).next;

//         if (prev != -1)
//         {
//             m_block_list.at(prev).next = next;
//         }
//         else
//         {
//             //! prev is head
//             assert(next != -1);
//             block_head = next;
//         }
//         if (next != -1) //! if next is -1 then block_ind is tail
//         {
//             m_block_list.at(next).prev = prev;
//         }
//     }

//     struct Block
//     {
//         std::shared_ptr<ComponentBlockI<ComponentType>> p_block;
//         int next;
//         int prev;
//         int free_count = BLOCK_SIZE;
//     };

// public:
//     int block_head = 0;
//     int free_spots_count = 0;
//     std::vector<Block> m_block_list;
// };