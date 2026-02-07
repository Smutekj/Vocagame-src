#pragma once

#include "DrawLayer.h"
#include "Utils/Grid.h"

#include "Commands.h"
#include "GameWorld.h"
#include "UISystem.h"
#include "Camera.h"
#include "TextureAtlas.h"
#include "Screen.h"
#include "FactoryBase.h"

#include "PostOffice.h"
#include "PostBox.h"
#include "Systems/TimedEventManager.h"

#include "ToolBoxUI.h"
#include "Menu/UIButtons.h"

#include "Components.h"
#include "Assets.h"

class GameWorld;

struct Track
{
    std::vector<utils::Vector2f> path;
    std::vector<utils::Vector2f> right_border;
    std::vector<utils::Vector2f> left_border;
};

class Editor : public Screen
{

    PostOffice messanger;

public:
    Editor(Renderer &window, KeyBindings &bindings, Assets &assets);
    virtual ~Editor() override {}

    virtual void update(float dt) final;
    virtual void handleEvent(const SDL_Event &event) final;
    virtual void draw(Renderer &window) final;

    void handleCameraMotion(const SDL_Event &event);
    void handleEventDesigner(const SDL_Event &event);
    void handleEventPath(const SDL_Event &event);
    void handleEventLevelSize(const SDL_Event &event);
    void parseInput(Renderer &window, float dt);

    static bool isKeyPressed(SDL_Keycode key);

    void synchonizeSpec(std::shared_ptr<GameObjectSpec> spec, GameObject &obj);

    void drawSizeArrows(Renderer &window);
    void drawBoundingBoxes(Renderer &window);
    void drawPath(Renderer &window);
    void drawGrid(Renderer &window);
    void drawDesigner(Renderer &window);

    template <class... TypeList>
    void createFactories();
    

    void initializeLayersAndTextures();
    void registerSystems();
    void restartGame();
    void initializeUI();
    void initializeSounds();

    std::unordered_map<std::string, std::shared_ptr<UIButtonI>> m_size_arrows;
    std::unordered_map<std::string, utils::Vector2f> m_arrows_dr;
    utils::Vector2f m_size_change_start_pos;
    utils::Vector2f m_old_size;

    Camera m_camera;
    utils::Vector2f m_camera_vel = {0.f, 0.f};
    bool m_dragging_camera = false;
    utils::Vector2f m_camera_drag_start_pos;
    glm::vec4 m_camera_drag_start_mouse_gl;


    Renderer &m_window;

    bool m_move_objects = false;
    std::unique_ptr<GameWorld> m_design_world;

    Assets& m_assets;
    TextureHolder &m_textures;
    TextureHolder2 &m_atlases;
    std::shared_ptr<Font> m_font;
    std::unique_ptr<Texture> m_background;

    LayersHolder m_layers;

    std::unordered_map<TypeId, std::unique_ptr<FactoryBase>> m_design_factories;

    TimedEventManager m_timers;

    ToolBoxUI m_ui;
    friend class ToolBoxUI;

    std::unordered_map<std::string, std::shared_ptr<UIButtonI>> m_buttons;

    Sprite m_background_sprite;

    std::unordered_set<std::size_t> m_selected_entity_ids;
    bool m_draw_bounding_boxes = false;
    //! Drag stuff
    utils::Vector2f m_drag_pos_start;
    utils::Vector2f m_old_obj_pos;
    bool m_is_dragging = false;

    //! grid stuff
    utils::Vector2f m_grid_cell_size = {50, 50};
    enum class SnapMode
    {
        Center,
        Grid,
        None,
    };
    SnapMode m_snap_mode = SnapMode::Center;

    std::function<void(std::shared_ptr<GameObjectSpec>)> m_on_spec_change_cb = [](std::shared_ptr<GameObjectSpec>) {};

    int m_moved_point_id = -1; //! if -1, no point is being moved;
    struct DesignedThing
    {
        utils::Vector2f m_origin = {0.f, 0.f};
        utils::Vector2f m_level_size = {800, 800};
        
        struct NamedPoint
        {
            utils::Vector2f coords;
            std::string name;
        };
        std::map<int, NamedPoint> m_named_positions;
        std::unordered_map<std::size_t, std::shared_ptr<GameObjectSpec>> m_things;

        void remove(std::size_t id, GameWorld &world)
        {
            if (m_things.contains(id))
            {
                m_things.erase(id);
                world.get(id)->kill();
            }
        }

        Rectf getBoundingBox() const
        {
            utils::Vector2f min_pos = {std::numeric_limits<float>::max()};
            utils::Vector2f max_pos = {std::numeric_limits<float>::min()};
            for (auto &[id, p_obj] : m_things)
            {
                auto pos = p_obj->pos;
                auto size = p_obj->size;
                min_pos.x = std::min(min_pos.x, pos.x - size.x / 2.f);
                min_pos.y = std::min(min_pos.y, pos.y - size.y / 2.f);
                max_pos.x = std::max(max_pos.x, pos.x + size.x / 2.f);
                max_pos.y = std::max(max_pos.y, pos.y + size.y / 2.f);
            }
            return Rectf{min_pos.x, min_pos.y, max_pos.x - min_pos.x, max_pos.y - min_pos.y};
        }
    };

    void resetDesign()
    {
        for (auto &[id, spec] : m_design.m_things)
        {
            m_design_world->get(id)->kill();
        }
        m_design.m_things.clear();
    }

    DesignedThing designFromJson(nlohmann::json &data);
    void addDesignToWorld(utils::Vector2f pos, const nlohmann::json &data);
    DesignedThing m_design;

    std::unordered_set<TypeId> m_designable_types;
    TypeId m_inserted_type = TypeId::Wall;

    std::unordered_map<std::size_t, ObjectType> m_grid2type;
    std::filesystem::path m_json_path;

    //! copying stuff
    std::shared_ptr<GameObjectSpec> m_copied_spec;
    TypeId m_copied_type;

    Path *m_path = nullptr;
    bool m_selecting_path = false;
    bool m_moving_path_point = false;
    int m_selected_pathstep = -1;

    bool m_moving_origin = false;
    void drawLevelBox(Renderer &window);

    Track track;
};

void serializeDesign(Editor::DesignedThing &design, std::string design_name, nlohmann::json &designs_data);