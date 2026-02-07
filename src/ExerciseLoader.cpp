#include "ExerciseLoader.h"

#include "Utils/IOUtils.h"
#include "Utils/RandomTools.h"
#include <nlohmann/json.hpp>

std::vector<TranslationExercise> TranslationExerciseLoader::loadExercises(std::string group, std::filesystem::path json_path)
{
    std::vector<TranslationExercise> repres;
    using json = nlohmann::json;

    auto exec_data = utils::loadJson(json_path.c_str());
    std::size_t group_size = 0;
    for (auto [index, item] : exec_data.at(group).items())
    {
        group_size++;
        TranslationExercise repre;
        repre.group = group;
        repre.type = item.value("type", "Noun");
        repre.image_id = item.value("image_id", "");
        repre.translation = item.at("translation");
        repre.correct_form = item.at("correct_word");
        repre.meaning = item.value("meaning_id", "");
        repres.push_back(repre);
    }

    return repres;
}

ExerciseGenerator::ExerciseGenerator(std::vector<TranslationExercise> &exers)
    : m_exercises(exers)
{
    for (auto &exer : exers)
    {
        if (!m_type2exercises.contains(exer.type))
        {
            m_type2exercises[exer.type] = {};
            m_types.push_back(exer.type);
        }

        m_type2exercises.at(exer.type).push_back(exer);
    }
}
TranslationExercise ExerciseGenerator::generate(bool correct)
{
    auto exer = m_exercises.at(m_current_index);
    m_current_index++;
    if (m_current_index >= m_exercises.size())
    {
        m_current_index = 0;
        utils::permute(m_exercises);
    }

    if (!correct)
    {
        //! generate translation of the same type
        auto suggested_word = utils::randomValue(m_type2exercises.at(exer.type));
        exer.shown_word = suggested_word.correct_form;
    }else{
        exer.shown_word = exer.correct_form;
    }

    return exer;
}