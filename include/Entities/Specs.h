#pragma once

#include <memory>

#include <Rect.h>

#include "../GameObject.h"

struct TextureId
{
    std::string texture_id;
    Recti tex_rect;
};

struct WordRepresentation
{

    std::string group;
    std::string type;
    std::string meaning_id;
    std::string translation;
    std::string correct_form;
    std::string image_id;

    std::vector<std::string> shown_forms;
};

enum class WordType
{
    Basic,
    Platform,
    Correct,
    Dodger
};

struct WordSpec : public GameObjectSpec
{
    WordSpec();

    using IdType = WordType;

    IdType type = WordType::Basic;

    std::string text = "Penis";
    std::string correct_form = "Penis";
    std::string translation = "Schlong";
};




// ##############################################//
// ##############################################//
// ##############################################//
// ##############################################//


