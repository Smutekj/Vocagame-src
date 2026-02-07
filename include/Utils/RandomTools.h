#pragma once

#include <random>
#include <iterator>

#include "../Vertex.h"

namespace utils
{

    inline float randf(float min = 0, float max = 1)
    {
        return (rand() / static_cast<float>(RAND_MAX)) * (max - min) + min;
    }

    inline Vec2 randomPosInBox(Vec2 ul_corner,
                               Vec2 box_size)
    {
        return {ul_corner.x + rand() / static_cast<float>(RAND_MAX) * box_size.x,
                ul_corner.y + rand() / static_cast<float>(RAND_MAX) * box_size.y};
    }

    inline int randi(int min, int max)
    {
        return rand() % (max - min + 1) + min;
    }

    inline int randi(int max)
    {
        return randi(0, max);
    }

    template <std::input_or_output_iterator Iter>
    inline auto randomValue(Iter begin,
                            Iter end)
    {
        std::size_t size = std::distance(end, begin);
        return *(begin + randi(0, size));
    }

    template <class T>
    inline auto randomValue(T &&values)
    {
        return values.at(randi(0, values.size() - 1));
    }

    class Random
    {
    public:
        Random()
            : rng(std::random_device{}())
        {
        }

        Random(int seed)
            : rng(seed)
        {
        }

        int getInt(int min, int max)
        {
            std::uniform_int_distribution<std::mt19937::result_type> dist(min, max); // distribution in range [1, 6]
            return dist(rng);
        }

    private:
        std::mt19937 rng;
    };

    template <class Type>
    void permute(std::vector<Type> &data)
    {
        std::size_t n = data.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            int rand_i = randi(i, n - 1);
            std::swap(data.at(i), data.at(rand_i));
        }
    };

    template <class Type>
    std::vector<Type> generatePermutation(const std::vector<Type> &data)
    {
        std::vector<Type> result = data;
        permute(result);
        return result;
    };

    inline std::vector<int> randomIndexPermutation(std::size_t count)
    {
        std::vector<int> cell_ids(count);
        std::size_t n(0);
        std::generate(std::begin(cell_ids), std::end(cell_ids), [&]
                      { return n++; });
        permute(cell_ids);
        return cell_ids;
    }

} //! namespace utils