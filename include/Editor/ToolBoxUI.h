#pragma once

#include <Window.h>
#include <Texture.h>
#include <Renderer.h>
#include <FrameBuffer.h>

#include <filesystem>
#include <unordered_set>
#include <queue>

#include "nlohmann/json.hpp"
#include "TextureAtlas.h"
#include "GameObject.h"

#include <SDL2/SDL_events.h>

class GameWorld;
class Editor;
class Player;

class ToolBoxUI
{
public:
       ToolBoxUI(Window &window, TextureHolder &textures, Editor *p_editor);
       ~ToolBoxUI();

       void draw();
       void initWorld(GameWorld &world);

       bool handleEvent(SDL_Event event);

       void drawResourceManager();
       void drawSoundOverview();
       void drawTextureOverview();
       void drawFileSelection(std::string &selected_file_name,
                              const std::vector<std::filesystem::path> &file_list,
                              const std::filesystem::path &directory_path,
                              std::function<void(std::string &)> on_selection);

       void drawDesignManager();
       void drawEntityDesigner();
       void drawPlayer();

       void redrawImage();

       std::shared_ptr<GameObjectSpec> getSelectedSpec();

       // template <class DataType>
       // bool drawSpecLine(DataType &value, const std::string &key_name);

       void registerSpecDrawers();

       template <class EntityType>
       void registerSpecDrawer()
       {

              using SpecType = typename EntityType::Spec;
              //! ui can draw only registered specs;
              if constexpr (boost::describe::has_describe_members<SpecType>::value)
              {
                     m_spec_drawers[typeid(SpecType)] = [](std::shared_ptr<GameObjectSpec> spec,
                                                           std::function<void(std::shared_ptr<GameObjectSpec>)> callback)
                     { drawSpec(std::static_pointer_cast<SpecType>(spec), callback); };

                     m_spec_cloners[typeid(SpecType)] = [](GameObjectSpec &spec)
                     {
                            auto &the_spec = static_cast<SpecType &>(spec);
                            auto clone = std::make_shared<SpecType>(the_spec);
                            return clone;
                     };
              }
       }
       template <class... TypeList>
       void registerSpecs();

       void drawSpriteUI(Sprite &sprite);
       void drawTheSpec(std::shared_ptr<GameObjectSpec> spec, std::function<void(std::shared_ptr<GameObjectSpec>)> callback = [](std::shared_ptr<GameObjectSpec>) {});

       std::unordered_map<std::type_index, std::function<void(std::shared_ptr<GameObjectSpec>, std::function<void(std::shared_ptr<GameObjectSpec>)>)>> m_spec_drawers;
       std::unordered_map<std::type_index, std::function<std::shared_ptr<GameObjectSpec>(GameObjectSpec &)>> m_spec_cloners;

       Player *p_player = nullptr;
       GameWorld *p_world = nullptr;
       Editor *p_editor = nullptr;

       bool show_demo_window = true;

       std::shared_ptr<GameObjectSpec> m_changing_spec;

       // bool m_selecting_texture = false;

       FrameBuffer m_sprite_pixels;
       Renderer m_sprite_canvas;

       std::string m_texture_id;

       enum class ResourceType
       {
              Texture,
              Font,
              Sound,
              Atlas,
              None
       };
       ResourceType m_active_resource = ResourceType::Texture;

       std::filesystem::path m_design_dir_path;
       std::string m_design_filename;

       bool m_selecting_design_file = true;
       std::vector<std::filesystem::path> m_design_file_list;
       std::string m_selected_design_filename;

       nlohmann::json designs_data;
       std::string selected_design_key;
       std::string new_design_name = "NewDesign";
       bool m_filesave_warning = false;

       std::string m_selected_sound_id = "";

       nlohmann::json resources_data;
};

void drawShaderUniforms(Shader &shader);