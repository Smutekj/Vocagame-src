#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>

template <class ExerType>
class ExerciseLoader
{

public:
    virtual std::vector<std::shared_ptr<ExerType>> loadExercises(std::string group, std::filesystem::path path) = 0;

private:
};

struct TrueFalseExercise
{
};

struct TranslationExercise : public TrueFalseExercise
{
    std::string correct_form;
    std::string shown_word;
    std::string translation;
    std::string meaning;

    std::string image_id;
    std::string type;

    std::string group;

    int score = 0;

    bool isCorrect() const
    {
        return correct_form == shown_word;
    };
};

class TranslationExerciseLoader
{
public:
    std::vector<TranslationExercise> loadExercises(std::string group, std::filesystem::path json_path);
};

struct PropertyDescription
{
    std::size_t property_id;
    std::string description;
};

struct TermsExercise
{
    std::string term = "Oxygen";

    std::unordered_set<std::string> properties;

    bool hasProperty(const std::string &property) const
    {
        return properties.count(property) > 0;
    }
};

class TermsExerciseLoader
{
public:
    std::vector<std::shared_ptr<TrueFalseExercise>> loadFromDb();
};

class ExerciseGenerator
{

public:
    ExerciseGenerator(std::vector<TranslationExercise> &exers);
    
    TranslationExercise generate(bool correct);

private:
    std::vector<TranslationExercise> m_exercises;
    std::unordered_map<std::string, std::vector<TranslationExercise>> m_type2exercises;
    std::vector<std::string> m_types;
    std::size_t m_current_index = 0;
};