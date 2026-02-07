#include "ToolBoxUI.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <imgui_stdlib.h>

#include <fstream>
#include <unordered_map>

#include <Window.h>
#include <Shader.h>
#include <Texture.h>

#include "Editor.h"
#include "GameWorld.h"
#include "Entities/Player.h"
#include "Entities/Factories.h"

#include "serializers.h"
#include "Utils/IOUtils.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

static bool m_selecting_texture = false;
static fs::path m_texture_directory = std::string{RESOURCES_DIR} + "Textures/";
static std::unordered_map<std::string, fs::path> m_tex_id2path;

//! animations
static std::unordered_map<std::string, fs::path> m_anim_id2path;

std::string m_selected_tex_id = "";
std::string m_selected_texture_name = "";

static std::vector<fs::path> m_texture_paths = {};
int m_selected_texfile_id = -1;

Texture *s_texture_canvas = nullptr;
Recti m_texture_rect;

LayersHolder *layers;
std::string m_selected_layer_id = "Unit";
std::string m_selected_shader_id = "Instanced";

Editor *p_editor = nullptr;

TextureHolder *m_textures;
TextureHolder2 m_atlases;

nlohmann::json loadDesign(const std::filesystem::path &path)
{
    if (!fs::exists(path) || path.extension() != ".json")
    {
        return {};
    }

    nlohmann::json data = utils::loadJson(path.c_str());
    return data;
}

ToolBoxUI::ToolBoxUI(Window &window, TextureHolder &textures, Editor *game)
    : m_sprite_pixels(400, 400, TextureOptions{.internal_format = TextureFormat::RGBA, .data_type = TextureDataTypes::UByte}),
      m_sprite_canvas(m_sprite_pixels), p_editor(game)
{
    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    const char *glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    const char *glsl_version = "#version 300 es";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char *glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();
    ImGuiStyle &style = ImGui::GetStyle();
    // style.ScaleAllSizes(10.f); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window.getHandle(), window.getContext());
    ImGui_ImplOpenGL3_Init(glsl_version);

    /*---------------------------------------*/

    m_sprite_canvas.m_view.setCenter(m_sprite_pixels.getSize() / 2.f);
    m_sprite_canvas.m_view.setSize(utils::Vector2f{m_sprite_pixels.getSize().x, -m_sprite_pixels.getSize().y});

    ::p_editor = game;
    layers = &p_editor->m_layers;

    m_textures = &textures;
    m_textures->setBaseDirectory(m_texture_directory);

    m_design_filename = "newWallShapes.json";
    m_design_dir_path = std::string{RESOURCES_DIR} + "Levels";
    designs_data = loadDesign(m_design_dir_path / m_design_filename);

    registerSpecDrawers();
}

ToolBoxUI::~ToolBoxUI()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

std::vector<fs::path> readAnimationsFromDirectory(std::filesystem::path directory_path)
{
    if (!fs::exists(directory_path) || !fs::is_directory(directory_path))
    {
        std::cout << "Cannot read animations in: " << directory_path.string() << " Path does not exist or is not directory!" << std::endl;
        return {};
    }

    std::vector<fs::path> texture_paths;

    std::cout << "Reading Animations in path: " << directory_path << std::endl;
    for (auto const &dir_entry : fs::directory_iterator{directory_path})
    {
        if (fs::path(dir_entry).extension() == ".png")
        {
            auto json_path = fs::path(dir_entry).replace_extension("json");
            bool json_exists = fs::exists(json_path);
            if (!json_exists)
            {
                std::cout << "Missing json file: " << json_path.c_str() << std::endl;
                continue;
            }

            texture_paths.push_back(dir_entry);
            auto anim_id = fs::path(dir_entry).replace_extension("").filename();
            m_anim_id2path[anim_id] = fs::path(dir_entry);
        }
    }

    return texture_paths;
}
std::vector<fs::path> readTexturesFromDirectory(std::filesystem::path directory_path)
{
    if (!fs::exists(directory_path) || !fs::is_directory(directory_path))
    {
        std::cout << "Cannot read textures in: " << directory_path.string() << " Path does not exist or is not directory!" << std::endl;
        return {};
    }

    std::vector<fs::path> texture_paths;

    std::cout << "Reading textures in path: " << directory_path << std::endl;
    for (auto const &dir_entry : fs::directory_iterator{directory_path})
    {
        if (fs::path(dir_entry).extension() == ".png")
        {
            texture_paths.push_back(dir_entry);
            auto tex_id = fs::path(dir_entry).replace_extension("").filename();
            m_tex_id2path[tex_id] = fs::path(dir_entry);
            if (!m_textures->get(tex_id))
            {
                m_textures->add(tex_id, fs::path(dir_entry).filename());
            }
        }
    }

    return texture_paths;
}

bool ToolBoxUI::handleEvent(SDL_Event event)
{
    ImGui_ImplSDL2_ProcessEvent(&event);

    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureKeyboard && event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
    {
        return true;
    }

    if (io.WantCaptureMouse &&
        (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP ||
         event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEWHEEL))
    {
        return true;
    }

    return false;

    /*     if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
        {
            ImVec2 mouse_pos = ImGui::GetIO().MousePos;
            // if (isInImage(mouse_pos))
            {
            }
         }*/
}

/* bool isInImage(ImVec2 point, Recti bounds)
{
    return point.x >= m_image_min.x && point.x < m_image_min.x + m_image_size.x &&
           point.y >= m_image_min.y && point.y < m_image_min.y + m_image_size.y;
}
 */
/* void ToolBoxUI::redrawImage()
{
    if (!m_textures->get(m_selected_texture_name))
    {
        return;
    }
    m_sprite_canvas.clear({0, 0, 0, 1});

    //! draw sprite into canvas;
    Sprite sprite(*m_textures->get(m_selected_texture_name));
    utils::Vector2f tex_size = m_textures->get(m_selected_texture_name)->getSize();
    float aspect_ratio = tex_size.y / tex_size.x;
    float element_aspect_ratio = m_image_size.y / m_image_size.x;

    m_image_size = {400, 400};
    utils::Vector2f image_size = {m_image_size.x, m_image_size.y * aspect_ratio};
    if (aspect_ratio > 1.f)
    {
        image_size = {m_image_size.y / aspect_ratio, m_image_size.y};
        sprite.setPosition(m_image_size.x / 2.f, image_size.y / 2.f);
    }
    else
    {
        image_size = {m_image_size.y, m_image_size.y * aspect_ratio};
        sprite.setPosition(image_size.x / 2.f, m_image_size.y / 2.f);
    }

    sprite.setScale(image_size.x / 2.f, image_size.y / 2.f);
    m_sprite_canvas.drawSprite(sprite);
    m_sprite_canvas.m_blend_factors = {BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha};
    m_sprite_canvas.drawAll();

    s_texture_canvas = &m_sprite_pixels.getTexture();
} */

template <template <class EnumType> class Container, class EnumType>
void drawSelection(const Container<EnumType> &allowed_selections, EnumType &selection, const std::string &box_name, std::function<void(ObjectType)> on_selection = [](ObjectType t) {})
{
    ImGui::Text("%s", box_name.c_str());
    if (ImGui::BeginListBox(box_name.c_str()))
    {
        for (auto &option : allowed_selections)
        {
            const bool is_selected = (option == selection);
            const auto option_name = std::string(enumToString(option));

            if (ImGui::Selectable(option_name.c_str(), is_selected))
            {
                selection = option;
                on_selection(option);
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
}
template <class EnumType>
bool drawEnumSelection(EnumType &selection, const std::string &box_name)
{
    bool value_changed = false;
    ImGui::Text("%s", box_name.c_str());
    if (ImGui::BeginListBox(box_name.c_str()))
    {

        const auto options_array = describe_enumerators_as_array<EnumType>();
        for (auto &[option_value, name] : options_array)
        {
            const bool is_selected = (option_value == selection);

            if (ImGui::Selectable(name, is_selected))
            {
                selection = option_value;
                value_changed = true;
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
    return value_changed;
}
void drawKeySelection(std::string &selected_key, json &data, std::function<void(nlohmann::json &)> on_selection)
{
    if (ImGui::BeginListBox("Select Key"))
    {
        for (auto [key, item] : data.items())
        {
            const bool is_selected = (selected_key == key.c_str());
            if (ImGui::Selectable(key.c_str(), is_selected))
            {
                selected_key = key.c_str();
                on_selection(item);
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
}

bool drawPathUI(Path &path)
{
    if (!p_editor->m_selecting_path)
    {
        return false;
    }
    bool value_changed = false;
    if (ImGui::Begin("Path"))
    {
        if (ImGui::Button("Close"))
        {
            p_editor->m_selecting_path = false;
        }

        value_changed |= ImGui::Checkbox("Cyclic", &path.cyclic);
        int selected_step = p_editor->m_selected_pathstep;
        ImGui::Text("Path Points");
        for (int pathstep = 0; pathstep < path.steps.size(); ++pathstep)
        {
            if (pathstep == selected_step)
            {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 0.8f, 0.2f, 0.5f)); // yellow-ish
            }
            auto &step = path.steps.at(pathstep);
            value_changed |= ImGui::InputFloat2(("Pos-" + std::to_string(pathstep)).c_str(), &step.target.x);
            value_changed |= ImGui::InputFloat(("Speed-" + std::to_string(pathstep)).c_str(), &step.speed);
            if (pathstep == selected_step)
            {
                ImGui::PopStyleColor();
            }
        }
    }
    ImGui::End();

    return value_changed;
}

template <class ContainerType, class KeyT, class ValueT>
bool drawUISelection(ContainerType &container, std::string selection_name, KeyT &selected_key, std::function<void(ValueT &)> on_selection = [](ValueT &) {})
{
    bool value_changed = false;
    if (ImGui::BeginListBox(selection_name.c_str()))
    {
        for (auto &[key, value] : container)
        {
            bool selected = (key == selected_key);
            if (ImGui::Selectable((value.name + "##").c_str(), &selected))
            {
                selected_key = key;
                on_selection(value);
                value_changed = true;
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
    return value_changed;
}

bool drawSpriteUI(SpriteSpec &sprite)
{
    bool value_changed = false;
    if (!m_selecting_texture)
    {
        return value_changed;
    }
    ImGui::Begin("Textures", &m_selecting_texture);
    if (ImGui::Button("Close"))
    {
        m_selecting_texture = false;
    }
    if (ImGui::BeginListBox("Texture Files"))
    {
        for (int id = 0; id < m_texture_paths.size(); ++id)
        {
            auto tex_filename = m_texture_paths.at(id).filename();
            const bool is_selected = (m_selected_tex_id == tex_filename.string());

            if (ImGui::Selectable(tex_filename.c_str(), is_selected))
            {
                m_selected_tex_id = tex_filename;
                if (!m_textures->get(tex_filename))
                {
                    auto path = tex_filename;
                    m_textures->add(tex_filename.replace_extension("").string(), path);
                }
                m_selected_texture_name = tex_filename.replace_extension("").string();

                auto tex_size = m_textures->get(tex_filename)->getSize();
                m_texture_rect = {0, 0, (int)tex_size.x, (int)tex_size.y};
                sprite.tex_rect = m_texture_rect;
                sprite.texture_id = m_selected_texture_name;
                // sprite.setTexture(*m_textures->get(tex_filename));

                value_changed = true;
            }
            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }

    int color[4] = {(int)sprite.color.r, (int)sprite.color.g, (int)sprite.color.b, (int)sprite.color.a};
    value_changed |= ImGui::InputInt4("Color", color);
    sprite.color = {(unsigned char)color[0], (unsigned char)color[1], (unsigned char)color[2], (unsigned char)color[3]};

    if (ImGui::BeginListBox("Layers"))
    {
        for (auto [layer_id, depth] : layers->m_name2depth)
        {
            bool selected = (layer_id == m_selected_layer_id);
            if (ImGui::Selectable(layer_id.c_str(), &selected))
            {
                m_selected_layer_id = layer_id;
                sprite.layer_id = layer_id;
                value_changed = true;
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
    auto canvas_p = layers->getCanvasP(m_selected_layer_id);
    auto &shaders = canvas_p->getShaders().getShaders();
    if (canvas_p && ImGui::BeginListBox("Shaders##2"))
    {
        for (auto [shader_id, shader] : shaders)
        {
            bool selected = (shader_id == m_selected_layer_id);
            if (ImGui::Selectable(shader_id.c_str(), &selected))
            {
                m_selected_shader_id = shader_id;
                sprite.shader_id = shader_id;
                value_changed = true;
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndListBox();
    }
    if (shaders.contains(m_selected_shader_id))
    {
        drawShaderUniforms(*shaders.at(m_selected_shader_id));
    }

    value_changed |= ImGui::Checkbox("Scalable", &sprite.scalable);

    if (s_texture_canvas)
    {
        auto selected_tex = m_textures->get(m_selected_texture_name);
        //! draw the texture itself
        utils::Vector2i tex_size = s_texture_canvas->getSize();
        float tex_aspec = (float)tex_size.y / (float)tex_size.x;
        utils::Vector2f im_size = {400.f};
        if (tex_aspec > 1.0)
        {
            im_size.x = im_size.y / tex_aspec;
        }
        else
        {
            im_size.y = im_size.x * tex_aspec;
        }
        value_changed |= ImGui::InputInt4("Tex Rect", &sprite.tex_rect.pos_x);
        ImGui::SameLine();
        bool repeats_x = selected_tex->getOptions().wrap_x == TexWrapParam::Repeat;
        bool repeats_y = selected_tex->getOptions().wrap_y == TexWrapParam::Repeat;
        if (ImGui::Checkbox("Repeat X", &repeats_x))
        {
            // sprite.options.wrap_x = repeats_x ? TexWrapParam::Repeat : TexWrapParam::ClampEdge;
            // sprite.options->setWrapX(param);
            value_changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Repeat Y", &repeats_y))
        {
            // sprite.options.wrap_y = repeats_y ? TexWrapParam::Repeat : TexWrapParam::ClampEdge;
            // sprite.m_texture->setWrapY(param);
            value_changed = true;
        }
        ImGui::Image((ImTextureID)(intptr_t)(s_texture_canvas->getHandle()), ImVec2(im_size.x, im_size.y), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(0, 1, 0, 1));
        float mouse_coords[2] = {0, 0};
        auto screen_cursor_pos = ImGui::GetCursorScreenPos();
        auto cursor_pos = ImGui::GetCursorPos();
        auto mouse_pos = ImGui::GetMousePos();
        ImGui::Text("%s %f %f", "Screen Cursor Pos: ", screen_cursor_pos.x, screen_cursor_pos.y);
        ImGui::Text("%s %f %f", "Cursor Pos: ", cursor_pos.x, cursor_pos.y);
        ImGui::Text("%s %f %f", "Mouse Pos: ", mouse_pos.x, mouse_pos.y);
    }

    ImGui::End();
    return value_changed;
}

std::unordered_map<std::string, int> ui_key_registry;
std::string registerKey(const std::string &ui_key)
{
    if (!ui_key_registry.contains(ui_key))
    {
        ui_key_registry[ui_key] = 0;
        return ui_key;
    }
    ui_key_registry.at(ui_key)++;
    return ui_key + "##" + std::to_string(ui_key_registry.at(ui_key));
}

template <class DataType>
constexpr bool drawSpecLine(DataType &value, const std::string &key_name)
{
    auto ui_key = registerKey(key_name);

    bool value_changed = false;
    if constexpr (std::is_same_v<DataType, Path>)
    {
        if (ImGui::Button("Select Path"))
        {
            p_editor->m_selecting_path = true;
            p_editor->m_path = &value;
            auto &selected_obj = *p_editor->m_design_world->get(*p_editor->m_selected_entity_ids.begin());
            // value.steps
            return false;
        }
        return drawPathUI(value);
    }
    if constexpr (std::is_same_v<DataType, SpriteSpec>)
    {
        if (ImGui::Button("Sprite"))
        {
            m_selecting_texture = !m_selecting_texture;
            if (m_selecting_texture)
            {
                m_texture_paths = readTexturesFromDirectory(m_texture_directory);
            }
            return false;
        }
        return drawSpriteUI(value);
    }
    if constexpr (std::is_same_v<DataType, float>)
    {
        return ImGui::InputFloat(ui_key.c_str(), &value);
    }
    else if constexpr (std::is_same_v<DataType, glm::vec3>)
    {
        return ImGui::InputFloat3(ui_key.c_str(), &value.x);
    }
    else if constexpr (std::is_same_v<DataType, glm::vec4>)
    {
        return ImGui::InputFloat4(ui_key.c_str(), &value.x);
    }
    else if constexpr (std::is_same_v<DataType, utils::Vector2f> || std::is_same_v<DataType, glm::vec2>)
    {
        return ImGui::InputFloat2(ui_key.c_str(), &value.x);
    }
    else if constexpr (std::is_same_v<DataType, utils::Vector2i>)
    {
        return ImGui::InputInt2(ui_key.c_str(), &value.x);
    }
    else if constexpr (std::is_same_v<DataType, int>)
    {
        return ImGui::InputInt(ui_key.c_str(), &value);
    }
    else if constexpr (std::is_same_v<DataType, bool>)
    {
        return ImGui::Checkbox(ui_key.c_str(), &value);
    }
    else if constexpr (std::is_same_v<DataType, std::string>)
    {
        return ImGui::InputText(ui_key.c_str(), &value);
    }
    else if constexpr (std::is_same_v<DataType, ColorByte>)
    {
        float color_f[4] = {value.r / (255.f), value.g / (255.f), value.b / (255.f), value.a / (255.f)};
        bool changed_value = ImGui::ColorPicker4(ui_key.c_str(), color_f);
        value = {(u_int8_t)(color_f[0] * 255), (u_int8_t)(color_f[1] * 255), (u_int8_t)(color_f[2] * 255), (u_int8_t)(color_f[3] * 255)};
        return changed_value;
    }
    else if constexpr (std::is_same_v<DataType, Color>)
    {
        ImGui::TextColored(ImVec4(value.r, value.g, value.b, value.a), "%s", ui_key.c_str());
        return ImGui::InputFloat4(ui_key.c_str(), &value.r);
    }
    else if constexpr (std::is_enum_v<DataType>)
    {
        if (key_name != "obj_type")
        {
            return drawEnumSelection(value, key_name);
        }
    }
    return false;
};

using namespace boost::describe;
template <class Type,
          class SpecMembersD = describe_members<Type, mod_any_access | mod_inherited>>
bool drawDescribed(Type &spec, int depth)
{
    bool spec_changed = false;
    boost::mp11::mp_for_each<SpecMembersD>([&](auto spec_member)
                                           { 
                                            using MemberType = std::decay_t<decltype(spec.*spec_member.pointer)>;
                                            if constexpr(std::is_base_of_v<GameObjectSpec, MemberType> && 
                                                         has_describe_members<MemberType>::value)
                                            {
                                                if(ImGui::CollapsingHeader(spec_member.name))
                                                {
                                                    spec_changed |= drawDescribed(spec.*spec_member.pointer, depth + 1);
                                                }
                                            }else{
                                                spec_changed |= drawSpecLine(spec.*spec_member.pointer, spec_member.name); 
                                            } });
    return spec_changed;
}

using namespace boost::describe;
template <class SpecType,
          class SpecMembersD = describe_members<SpecType, mod_any_access | mod_inherited>>
void drawSpec(std::shared_ptr<SpecType> spec_p, std::function<void(std::shared_ptr<GameObjectSpec>)> callback)
{
    bool spec_changed = drawDescribed(*spec_p, 0);
    if (spec_changed)
    {
        callback(spec_p);
    }
}

std::shared_ptr<GameObjectSpec> ToolBoxUI::getSelectedSpec()
{
    return m_spec_cloners.at(m_changing_spec->rtti)(*m_changing_spec);
}

void saveDesign(const nlohmann::json &designs_data, const std::filesystem::path &path)
{
    std::ofstream file_json(path);
    file_json << designs_data << std::endl;
}

std::vector<std::filesystem::path> getFileList(const std::filesystem::path &directory, const std::string &name_suffix)
{
    std::vector<std::filesystem::path> file_paths;

    if (!fs::exists(directory) || !fs::is_directory(directory))
    {
        std::cout << directory << " Does not exist or not a directory!" << std::endl;
        return {};
    }

    for (const auto &dir_entry : fs::directory_iterator(directory))
    {
        const auto &entry_path = fs::path(dir_entry);
        if (entry_path.filename().string().ends_with(name_suffix))
        {
            file_paths.push_back(entry_path);
        }
    }
    return file_paths;
}

void ToolBoxUI::drawFileSelection(std::string &selected_file_name,
                                  const std::vector<fs::path> &file_list,
                                  const fs::path &directory_path,
                                  std::function<void(std::string &)> on_selection)
{
    namespace fs = std::filesystem;

    if (ImGui::Button("Refresh Designs"))
    {
        std::filesystem::path resources_dir = std::string{RESOURCES_DIR} + "Levels/";
        m_design_file_list = getFileList(resources_dir, ".json");
    }
    ImGui::SameLine();
    if (ImGui::Button("Close"))
    {
        m_selecting_design_file = false;
    }

    if (ImGui::BeginListBox("Select File"))
    {

        for (const auto &dir_path : file_list)
        {
            const auto &entry_name = dir_path.filename();
            bool is_selected = selected_file_name == entry_name;
            if (ImGui::Selectable(entry_name.c_str(), &is_selected))
            {
                selected_file_name = entry_name;
                on_selection(selected_file_name);
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndListBox();
    }
}

void ToolBoxUI::drawEntityDesigner()
{

    ImGui::Begin("Entity Designer");

    if (!m_selecting_design_file && ImGui::Button("Select Design"))
    {
        m_selecting_design_file = true;
    }

    ImGui::Checkbox("Start Time", &p_editor->m_move_objects);
    ImGui::SameLine();
    ImGui::Checkbox("Show Entity Bounds", &p_editor->m_draw_bounding_boxes);
    ImGui::InputFloat2("Level Size", &p_editor->m_design.m_level_size.x);

    ImGui::NewLine();
    ImGui::InputFloat2("Grid Size", &p_editor->m_grid_cell_size.x);

    auto snap_mode = p_editor->m_snap_mode;
    if (ImGui::RadioButton("Snap To Center", snap_mode == Editor::SnapMode::Center))
    {
        p_editor->m_snap_mode = Editor::SnapMode::Center;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Snap To Grid", snap_mode == Editor::SnapMode::Grid))
    {
        p_editor->m_snap_mode = Editor::SnapMode::Grid;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("No Snap", snap_mode == Editor::SnapMode::None))
    {
        p_editor->m_snap_mode = Editor::SnapMode::None;
    }
    ImGui::NewLine();

    //! named points
    auto &named_points = p_editor->m_design.m_named_positions;
    if (ImGui::Button("Add new point"))
    {
        int new_id = named_points.empty() ? 0 : named_points.rbegin()->first + 1;
        named_points.insert({new_id, {{0, 0}, "Next Point"}});
    }
    //! how to deduce the type???
    using NP = Editor::DesignedThing::NamedPoint;
    std::function<void(NP &)> cb = [](NP &point) {};
    drawUISelection(named_points, std::string("Named Points"), p_editor->m_moved_point_id, cb);

    if (named_points.contains(p_editor->m_moved_point_id))
    {
        int id = p_editor->m_moved_point_id;
        auto &point = named_points.at(id);
        ImGui::InputText(("##NamedPointName: " + std::to_string(id)).c_str(), &point.name);
        ImGui::SameLine();
        ImGui::InputFloat2(("##NamedPointCoords" + std::to_string(id)).c_str(), &point.coords.x);
    }

    drawSelection(p_editor->m_designable_types, p_editor->m_inserted_type, "Select Insertion Type",
                  [this](ObjectType selected_type)
                  {
                      m_changing_spec = p_editor->m_design_factories.at(selected_type)->makeSpec();
                  });

    ImGui::Text("Object Properties");
    if (m_changing_spec)
    {
        drawTheSpec(m_changing_spec, p_editor->m_on_spec_change_cb);
    }

    ImGui::End();

    if (m_selecting_texture)
    {
    }
}

void drawWarningPopup(std::string warning_text, std::function<void()> on_confirm, std::function<void()> on_cancel)
{
    ImGui::Begin("WARNING");
    ImGui::Text("%s", warning_text.c_str());
    if (ImGui::Button("Confirm"))
    {
        on_confirm();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
    {
        on_cancel();
    }
    ImGui::End();
}

void ToolBoxUI::drawDesignManager()
{
    if (m_selecting_design_file)
    {
        ImGui::Begin("Design Manager", &m_selecting_design_file);

        drawFileSelection(m_selected_design_filename, m_design_file_list, "Levels", [this](auto &)
                          { designs_data = loadDesign(m_design_dir_path / m_selected_design_filename);
                        m_design_filename = m_selected_design_filename; });

        if (!m_selected_design_filename.empty())
        {

            ImGui::InputText("Design Name", &m_design_filename);
            ImGui::Text("%s", "Select Design");
            drawKeySelection(selected_design_key, designs_data,
                             [this](nlohmann::json &design_json)
                             {
                                 new_design_name = selected_design_key;
                                 p_editor->resetDesign();
                                 p_editor->m_design = p_editor->designFromJson(design_json);
                                 p_editor->m_selected_entity_ids.clear();
                             });
            ImGui::InputText("Design name", &new_design_name);

            if (ImGui::Button("Clear Design"))
            {
                p_editor->resetDesign();
            }
            ImGui::SameLine();
            if (ImGui::Button("Rename Design"))
            {
                if (!designs_data.contains(new_design_name))
                {
                    auto design = designs_data[selected_design_key];
                    designs_data[new_design_name] = design;
                    designs_data.erase(selected_design_key);
                    selected_design_key = new_design_name;
                }
                else
                {
                    std::cout << "Design name: " << new_design_name << " exists!" << std::endl;
                }
            }

            if (ImGui::Button("save Design"))
            {
                serializeDesign(p_editor->m_design, new_design_name, designs_data);

                bool filename_exists = false;
                for (const auto &path : m_design_file_list)
                {
                    if (m_design_filename == path.filename())
                    {
                        filename_exists = true;
                        break;
                    }
                }
                if (filename_exists)
                {
                    m_filesave_warning = true;
                }
                else
                {
                    saveDesign(designs_data, m_design_dir_path / m_design_filename);
                    selected_design_key = new_design_name;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete Design"))
            {
                designs_data.erase(selected_design_key);
                saveDesign(designs_data, m_design_dir_path / m_design_filename);
                if (designs_data.size() > 0) //! select another design
                {
                    selected_design_key = designs_data.items().begin().key();
                }
                p_editor->m_selected_entity_ids.clear();
            }
        }
        ImGui::End();

        if (m_filesave_warning)
        {
            auto confirm = [this]()
            {
                saveDesign(designs_data, m_design_dir_path / m_design_filename);
                selected_design_key = new_design_name;
                m_filesave_warning = false;
            };
            auto cancel = [this]()
            { m_filesave_warning = false; };
            std::string warning_text = "Design file with name: " + new_design_name + " exists!";
            drawWarningPopup(warning_text, confirm, cancel);
        }
    }
}

void ToolBoxUI::drawTextureOverview()
{

    if (ImGui::Button("Refresh Images"))
    {
        std::filesystem::path resources_dir = std::string{RESOURCES_DIR} + "Textures/";
        m_texture_paths = readTexturesFromDirectory(resources_dir);
    }

    if (ImGui::InputText("Tex Id: ", &m_texture_id))
    {
        const auto tex_path = m_tex_id2path.at(m_selected_tex_id);
        m_tex_id2path.erase(m_selected_tex_id);
        auto tex = m_textures->get(m_selected_tex_id);
        m_textures->erase(m_selected_tex_id);
        m_selected_tex_id = m_texture_id;
        m_tex_id2path[m_selected_tex_id] = tex_path;
        m_textures->add(m_selected_tex_id, *tex);
    }
    if (auto selected_tex = m_textures->get(m_texture_id); selected_tex)
    {
        ImGui::SameLine();
        bool repeats_x = selected_tex->getOptions().wrap_x == TexWrapParam::Repeat;
        bool repeats_y = selected_tex->getOptions().wrap_y == TexWrapParam::Repeat;
        if (ImGui::Checkbox("Repeat X", &repeats_x))
        {
            auto wrap_x = repeats_x ? TexWrapParam::Repeat : TexWrapParam::ClampEdge;
            selected_tex->setWrapX(wrap_x);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Repeat Y", &repeats_y))
        {
            auto wrap_y = repeats_y ? TexWrapParam::Repeat : TexWrapParam::ClampEdge;
            selected_tex->setWrapX(wrap_y);
        }
    }
    if (ImGui::BeginListBox("Textures"))
    {
        for (auto &[id, tex] : m_textures->getTextures())
        {
            bool is_selected = id == m_selected_tex_id;
            if (ImGui::Selectable(id.c_str(), &is_selected))
            {
                m_selected_tex_id = id;
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }

    if (ImGui::Button("Clone"))
    {
        auto new_texture = std::make_shared<Texture>(*m_textures->get(m_selected_tex_id));
        m_textures->add(m_selected_tex_id + "-new", *new_texture);
    }
    if (ImGui::Button("Save Texture Json"))
    {
        nlohmann::json texture_json = {};
        for (auto &[id, tex] : m_textures->getTextures())
        {
            TextureSpec tex_spec{.id = id, .path = m_tex_id2path.at(id), .options = tex->getOptions()};
            texture_json[id] = serialize(tex_spec);
        }
        std::ofstream file_json("../Resources.json");
        resources_data["Texture"] = texture_json;
        file_json << resources_data << std::endl;
    }
}

#include "SoundSystem.h"

void ToolBoxUI::drawSoundOverview()
{
    if (ImGui::BeginListBox("Sound IDs"))
    {
        for (auto &[id, chunk] : SoundSystem::getSounds())
        {
            bool selected = id == m_selected_sound_id;
            if (ImGui::Selectable(id.c_str(), &selected))
            {
                m_selected_sound_id = id;
            }
            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }

    if (!m_selected_sound_id.empty())
    {
        if (ImGui::Button("Play Sound"))
        {
            SoundSystem::play(m_selected_sound_id);
        }
    }
}

bool m_resource_manager_open = true;

void ToolBoxUI::drawResourceManager()
{

    if (ImGui::Begin("Resource Manager", &m_resource_manager_open))
    {

        if (m_active_resource == ResourceType::Texture)
        {
            drawTextureOverview();
        }
        else if (m_active_resource == ResourceType::Sound)
        {
            drawSoundOverview();
        }

        if (m_active_resource == ResourceType::None)
        {
            if (ImGui::Button("Textures"))
            {
                m_active_resource = ResourceType::Texture;
            }
            ImGui::SameLine();
            if (ImGui::Button("Sounds"))
            {
                m_active_resource = ResourceType::Sound;
            }
        }
        else
        {
            if (ImGui::Button("Close"))
            {
                m_active_resource = ResourceType::None;
            }
        }
    }
    ImGui::End();
}

void ToolBoxUI::draw()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    drawResourceManager();

    ImGui::Begin("Shaders");
    {
        auto &shaders = p_editor->m_layers.getCanvas("Unit").getShaders().getShaders();

        if (ImGui::BeginListBox("Select Shader"))
        {
            for (auto &[key, shader] : shaders)
            {
                bool is_selected = key == m_selected_shader_id;
                if (ImGui::Selectable(key.c_str(), &is_selected))
                {
                    m_selected_shader_id = key;
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndListBox();
        }

        if (shaders.contains(m_selected_shader_id))
        {
            drawShaderUniforms(*shaders.at(m_selected_shader_id));
        }
        ImGui::End();
    }

    drawDesignManager();
    drawEntityDesigner();

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // redrawImage();

    ui_key_registry.clear();
}

void ToolBoxUI::initWorld(GameWorld &world)
{
    p_world = &world;
    p_player = world.m_player;
}

void ToolBoxUI::drawTheSpec(std::shared_ptr<GameObjectSpec> spec, std::function<void(std::shared_ptr<GameObjectSpec>)> callback)
{
    const auto &spec_type_id = typeid(*spec);
    if (m_spec_drawers.contains(spec->rtti))
    {
        m_spec_drawers.at(spec->rtti)(spec, callback);
    }
}


template <class... TypeList>
void ToolBoxUI::registerSpecs()
{
    (registerSpecDrawer<TypeList>(), ...);
}

void ToolBoxUI::registerSpecDrawers()
{
    registerSpecs<TYPE_LIST>();
}

void drawShaderUniforms(Shader &shader)
{
    auto uniforms = shader.getVariables().uniforms;

    for (auto &[key, uniform] : uniforms)
    {
        auto draw_ui_element = [&key, &shader, &uniform](auto &&v)
        {
            using T = std::decay_t<decltype(v)>;
            if (drawSpecLine<T>(v, key))
            {
                shader.setUniform(key, v);
                uniform.needs_update = true;
            }
        };
        std::visit(draw_ui_element, uniform.value);
    }
}