#include "Editor.h"

#include "Entities/Player.h"
#include "Entities/Bullet.h"
#include "Entities/Factories.h"

#include "Utils/RandomTools.h"

#include "Systems/TimedEventSystem.h"
#include "Systems/SpriteSystem.h"
#include "Systems/PathSystem.h"
#include "GameObjectSpec.h"

#include "SoundSystem.h"

#include "serializers.h"

#include <glm/trigonometric.hpp>
#include <Shader.h>

void Editor::initializeLayersAndTextures()
{

    auto width = m_window.getTargetSize().x;
    auto height = m_window.getTargetSize().y;

    TextureOptions options;
    options.wrap_x = TexWrapParam::ClampEdge;
    options.wrap_y = TexWrapParam::ClampEdge;
    options.mag_param = TexMappingParam::Linear;
    options.min_param = TexMappingParam::Linear;

    TextureOptions text_options;
    text_options.data_type = TextureDataTypes::UByte;
    text_options.format = TextureFormat::RGBA;
    text_options.internal_format = TextureFormat::RGBA;
    text_options.mag_param = TexMappingParam::Linear;
    text_options.min_param = TexMappingParam::Linear;

    std::filesystem::path shaders_directory = std::string{RESOURCES_DIR} + "Shaders/";
    double dpr = 1.;

    auto &unit_layer = m_layers.addLayer("Unit", 3, text_options, width, height);
    unit_layer.m_canvas.setShadersPath(shaders_directory);
    unit_layer.m_canvas.addShader("ElectroWall", "basicinstanced.vert", "ElectroWall.frag");
    unit_layer.m_canvas.addShader("lightningBolt", "basicinstanced.vert", "lightningBolt.frag");
    unit_layer.m_canvas.addShader("flag", "basicinstanced.vert", "flag.frag");
    unit_layer.m_canvas.getShader("TextDefault").setUniform("u_edge_color", glm::vec4{0, 0, 0, 1});

    auto &light_layer = m_layers.addLayer("Light", 4, text_options, width, height);
    light_layer.m_canvas.setShadersPath(shaders_directory);
    light_layer.m_canvas.addShader("Light", "basicinstanced.vert", "Light.frag");

    std::cout << "Unit layer dimensions: " << width << " : " << height << std::endl;
    auto &bloom_layer = m_layers.addLayer("Bloom", 5, options, width / dpr, height / dpr, dpr);
    bloom_layer.m_canvas.setShadersPath(shaders_directory);
    bloom_layer.m_canvas.addShader("ElectroWall", "basicinstanced.vert", "ElectroWall.frag");
    bloom_layer.m_canvas.addShader("gradientX", "basictex.vert", "gradientX.frag");
    bloom_layer.m_canvas.addShader("gradientY", "basictex.vert", "gradientY.frag");
    bloom_layer.addEffect(std::make_unique<BloomPhysical>(width / dpr, height / dpr, 2, 2, options));

    m_window.setShadersPath(shaders_directory);
    m_window.addShader("Shiny", "basicinstanced.vert", "shiny.frag");
    m_window.addShader("Arrow", "basicinstanced.vert", "texture.frag");
    m_window.addShader("LastPass", "basicinstanced.vert", "lastPass.frag");
    m_window.addShader("flag", "basicinstanced.vert", "flag.frag");
    m_window.addShader("healthBar", "basicinstanced.vert", "healthBar.frag");
    m_window.addShader("bossHealthBar", "basicinstanced.vert", "bossHealthBar.frag");
    m_window.addShader("boostBar", "basicinstanced.vert", "boostBar.frag");
    m_window.addShader("fuelBar", "basicinstanced.vert", "fuelBar.frag");
    m_window.addShader("boostBar2", "basicinstanced.vert", "boostBar2.frag");

    m_layers.getLayer("Unit")->setBackground({0.f, 0.f, 0.f, 0.f});
    m_layers.getLayer("Bloom")->setBackground({0.f, 0.f, 0.f, 0.f});
}

using namespace utils;

constexpr utils::Vector2f PLAYER_START_POS = {100, 20};
constexpr float START_VIEW_SIZE = 1000.f;

void Editor::restartGame()
{
}

void Editor::drawPath(Renderer &window)
{
    if (m_path->steps.size() == 0 || m_selected_entity_ids.empty())
    {
        return;
    }
    //! draw lines
    std::size_t steps_count = m_path->steps.size();

    auto &selected_obj = *m_design_world->get(*m_selected_entity_ids.begin());

    for (std::size_t pathstep = 0; pathstep < steps_count - !m_path->cyclic; ++pathstep)
    {
        auto &step = m_path->steps.at(pathstep).target;
        auto &next_step = m_path->steps.at((pathstep + 1) % steps_count).target;

        window.drawLineBatched(step + selected_obj.getPosition(), next_step + selected_obj.getPosition(), 3, {1, 0, 1, 1});
    }
    for (std::size_t pathstep = 0; pathstep < steps_count; ++pathstep)
    {
        Color c = pathstep == m_selected_pathstep ? Color{0, 1, 0, 1} : Color{0, 1, 1, 1};
        RectangleSimple r(c);
        auto &step = m_path->steps.at(pathstep).target;
        r.setPosition(step + selected_obj.getPosition());
        r.setScale(10.f, 10.f);
        window.drawRectangle(r);
    }
};

void Editor::initializeUI()
{
    m_buttons.clear();

    //! init UI
    auto window_size = m_window.getTargetSize();
    float button_size = window_size.y / 6.f;
    float button_offset = window_size.x / 10.f;
    float button_space = button_size / 1.5f;
    float l_pos_x = button_offset;
    float l_pos_y = window_size.y * 1.f / 8.f;
    float r_pos_x = l_pos_x + button_size + button_space;
    float r_pos_y = l_pos_y;
    utils::Vector2f exit_pos = {window_size.x * 7.f / 8.f, window_size.y * 7. / 8.};
    auto left_button = std::make_shared<IconButton>(*m_textures.get("Arrow"));
    auto right_button = std::make_shared<IconButton>(*m_textures.get("Arrow"));
    auto exit_button = std::make_shared<IconButton>(*m_textures.get("UIClose"));
    left_button->setScale(utils::Vector2f{button_size / 2.f});
    right_button->setScale(utils::Vector2f{button_size / 2.f});
    exit_button->setScale(utils::Vector2f{button_size / 6.f});
    left_button->setRotation(utils::radians(180.f));
    left_button->setPosition(l_pos_x, l_pos_y);
    right_button->setPosition(r_pos_x, r_pos_y);
    exit_button->setPosition(exit_pos);
    left_button->setOnClick([this]()
                            { m_camera.zoom(0.9f); });
    right_button->setOnClick([this]()
                             { m_camera.zoom(1.f / 0.9f); });
    exit_button->setOnClick([]() {});
    exit_button->setOnRelease([this]() {});
    m_buttons["leftButton"] = left_button;
    m_buttons["rightButton"] = right_button;
    m_buttons["exitButton"] = exit_button;
}

Editor::Editor(Renderer &window, KeyBindings &bindings, Assets &assets)
    : m_window(window),
      m_assets(assets),
      m_textures(assets.textures),
      m_font(assets.fonts.begin()->second),
      m_atlases(assets.atlases),
      m_ui(static_cast<Window &>(m_window.getTarget()), assets.textures, this),
      m_camera({PLAYER_START_POS.x, 0.f}, {START_VIEW_SIZE, START_VIEW_SIZE * window.getTargetSize().y / window.getTargetSize().x}, messanger)
{
    m_textures.get("SkyNight2")->setWrapX(TexWrapParam::Repeat);

    std::cout << "Creating Editor!" << std::endl;
    m_camera.threshold_rect = {0.0f, 0.00f, 0.0f, 0.00f};
#if defined(__EMSCRIPTEN__)
    // p_camera = &m_camera;
    // p_game = this;
    // setupDeviceOrientation();
#endif

    initializeLayersAndTextures();
    m_design_world = std::make_unique<GameWorld>(messanger);

    //! PLAYER NEEDS TO BE FIRST BECAUSE OTHER OBJECTS MIGHT REFERENCE IT!
    // m_player_system = std::make_unique<PlayerSystem>(*m_world, "../savefile.json");

    createFactories<TYPE_LIST>();
    registerSerializers();

    registerSystems();
    auto arrow_u = std::make_shared<IconButton>(*m_textures.get("Arrow"));
    arrow_u->setOnClick([this]()
                        { m_size_change_start_pos = m_window.getMouseInWorld(); });
    arrow_u->setScale(10.f, 10.f);
    arrow_u->setRotation(utils::radians(90.f));
    auto arrow_r = std::make_shared<IconButton>(*m_textures.get("Arrow"));
    arrow_r->setOnClick([this]()
                        { m_size_change_start_pos = m_window.getMouseInWorld(); });
    arrow_r->setScale(10.f, 10.f);
    arrow_r->setRotation(utils::radians(0.f));
    auto arrow_d = std::make_shared<IconButton>(*m_textures.get("Arrow"));
    arrow_d->setOnClick([this]()
                        { m_size_change_start_pos = m_window.getMouseInWorld(); });
    arrow_d->setScale(10.f, 10.f);
    arrow_d->setRotation(utils::radians(-90.f));
    auto arrow_l = std::make_shared<IconButton>(*m_textures.get("Arrow"));
    arrow_l->setOnClick([this]()
                        { m_size_change_start_pos = m_window.getMouseInWorld(); });
    arrow_l->setScale(10.f, 10.f);
    arrow_l->setRotation(utils::radians(180.f));
    m_arrows_dr["up"] = {0.f, 1.f};
    m_arrows_dr["right"] = {1.f, 0.f};
    m_arrows_dr["down"] = {0.f, -1.f};
    m_arrows_dr["left"] = {-1.f, 0.f};

    m_size_arrows["up"] = arrow_u;
    m_size_arrows["right"] = arrow_r;
    m_size_arrows["down"] = arrow_d;
    m_size_arrows["left"] = arrow_l;

    m_design.m_named_positions[0] = {{0, 0}, "origin"};
}

using json = nlohmann::json;

// static std::vector<CaveSpec> readCaveSpecs(std::filesystem::path json_file_path)
// {
//     std::ifstream json_file(json_file_path);
//     auto cave_data = json::parse(json_file);

//     std::vector<CaveSpec> specs;
//     for(auto& [key, value] : cave_data.items())
//     {
//         specs.push_back({});

//         for (auto &wall_coords_json : value.at("wall"))
//         {
//             int ix = wall_coords_json[0].get<int>();
//             int iy = wall_coords_json[1].get<int>();
//             specs.back().wall_coords.push_back({ix, iy});
//         }
//         for (auto &wall_coords_json : value.at("word"))
//         {
//             int ix = wall_coords_json[0].get<int>();
//             int iy = wall_coords_json[1].get<int>();
//             specs.back().word_coords = {ix, iy};
//         }
//     }
//     return specs;
// }

Rectf getBoundingRect(GameObject &obj)
{
    Rectf rect;
    rect.pos_x = obj.getPosition().x - obj.getSize().x / 2.f;
    rect.pos_y = obj.getPosition().y - obj.getSize().y / 2.f;
    rect.width = obj.getSize().x;
    rect.height = obj.getSize().y;
    return rect;
};

void drawBoundingBox(utils::Vector2f pos, utils::Vector2f scale, Renderer &window, Color color, float thickness = 2.f)
{
    window.drawLineBatched({pos.x + scale.x, pos.y + scale.y}, {pos.x + scale.x, pos.y - scale.y}, thickness, color);
    window.drawLineBatched({pos.x + scale.x, pos.y - scale.y}, {pos.x - scale.x, pos.y - scale.y}, thickness, color);
    window.drawLineBatched({pos.x - scale.x, pos.y - scale.y}, {pos.x - scale.x, pos.y + scale.y}, thickness, color);
    window.drawLineBatched({pos.x - scale.x, pos.y + scale.y}, {pos.x + scale.x, pos.y + scale.y}, thickness, color);
}

void drawBoundingBox(Rectf box, Renderer &window, Color color)
{
    utils::Vector2f scale = {box.width / 2.f, box.height / 2.f};
    utils::Vector2f pos = {box.pos_x, box.pos_y};
    drawBoundingBox(pos + scale, scale, window, color);
}

void drawBoundingBox(GameObject &obj, Renderer &window, Color color)
{
    utils::Vector2f scale = obj.getSize() / 2.f;
    utils::Vector2f pos = obj.getPosition();
    float angle_r = utils::radians(obj.getAngle());
    float sa = std::sin(angle_r);
    float ca = std::cos(angle_r);

    std::vector<utils::Vector2f> points = {{pos.x + scale.x, pos.y + scale.y},
                                           {pos.x + scale.x, pos.y - scale.y},
                                           {pos.x - scale.x, pos.y - scale.y},
                                           {pos.x - scale.x, pos.y + scale.y}};

    for (auto &point : points)
    {
        point -= pos;
        point = {ca * point.x - sa * point.y, sa * point.x + ca * point.y};
        point += pos;
    }
    for (int point_id = 0; point_id < points.size(); ++point_id)
    {
        auto curr_point = points.at(point_id);
        auto next_point = points.at((point_id + 1) % points.size());
        window.drawLineBatched(curr_point, next_point, 2.5, color);
    }
}
utils::Vector2f getGridSnappedPos(utils::Vector2f pos, utils::Vector2f grid_cell_size, Editor::SnapMode mode = Editor::SnapMode::Center)
{
    switch (mode)
    {
    case Editor::SnapMode::None:
    {
        return pos;
    }
    case Editor::SnapMode::Center:
    {
        utils::Vector2f grid_coords = {std::floor(pos.x / grid_cell_size.x), std::floor(pos.y / grid_cell_size.y)};
        return {(grid_coords.x + 0.5) * grid_cell_size.x, (grid_coords.y + 0.5) * grid_cell_size.y};
    }
    case Editor::SnapMode::Grid:
    {
        utils::Vector2f grid_coords = {std::floor(0.5 + pos.x / grid_cell_size.x), std::floor(0.5 + pos.y / grid_cell_size.y)};
        return {(grid_coords.x) * grid_cell_size.x, (grid_coords.y) * grid_cell_size.y};
    }
    default:
    {
        return pos;
    }
    }
}

void Editor::handleEventLevelSize(const SDL_Event &event)
{
    auto mouse_pos = m_window.getMouseInWorld();
}
void Editor::handleEventPath(const SDL_Event &event)
{
    auto mouse_position = m_window.getMouseInWorld();

    //! cannot add path if no object selected!
    if (m_selected_entity_ids.empty())
    {
        return;
    }
    auto &selected_obj = *m_design_world->get(*m_selected_entity_ids.begin());

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
    {
        for (std::size_t pathstep = 0; pathstep < m_path->steps.size(); pathstep++)
        {
            auto path_point_world = m_path->steps.at(pathstep).target + selected_obj.getPosition();
            float click_distance = utils::dist(path_point_world, mouse_position);
            if (click_distance < 15.f)
            {
                m_moving_path_point = true;
                m_selected_pathstep = pathstep;
                break;
            }
        }
    }

    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
    {
        m_moving_path_point = false;
        // m_selected_pathstep = -1;
    }
    //! move selected step
    if (event.type == SDL_MOUSEMOTION && m_moving_path_point)
    {
        auto path_pos_in_world = getGridSnappedPos(mouse_position, m_grid_cell_size, m_snap_mode);
        m_path->steps.at(m_selected_pathstep).target = path_pos_in_world - selected_obj.getPosition();
        // m_on_spec_change_cb(m_ui.m_changing_spec);
    }
    //! insert next step
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
    {
        PathStep new_step;
        new_step.target = getGridSnappedPos(mouse_position, m_grid_cell_size, m_snap_mode);
        new_step.target -= selected_obj.getPosition();
        m_path->steps.push_back(new_step);
        // m_on_spec_change_cb(m_ui.m_changing_spec);
    }

    //! unselect step
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
    {
        m_selected_pathstep = -1;
    }
    //! remove selected step
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_DELETE &&
        m_selected_pathstep >= 0 && m_selected_pathstep < m_path->steps.size())
    {
        m_path->steps.erase(m_path->steps.begin() + m_selected_pathstep);
        m_selected_pathstep = -1;
        // m_on_spec_change_cb(m_ui.m_changing_spec);
    }
}
void Editor::handleEventDesigner(const SDL_Event &event)
{
    auto old_view = m_window.m_view;

    auto mouse_position = m_window.getMouseInWorld();
    utils::Vector2f grid_center_pos = getGridSnappedPos(mouse_position, m_grid_cell_size);

    if (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_DELETE))
    {
        for (auto &id : m_selected_entity_ids)
        {
            m_design.remove(id, *m_design_world);
        }
        m_selected_entity_ids.clear();
    }

    auto posIsEmpty = [this](const utils::Vector2f &pos) -> bool
    {
        for (auto &p_obj : m_design_world->getEntities().data())
        {
            if (getBoundingRect(*p_obj).contains(pos))
            {
                return false;
            }
        }
        return true;
    };

    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_c && isKeyPressed(SDLK_LCTRL))
    {
        m_copied_spec = m_ui.m_spec_cloners.at(m_ui.m_changing_spec->rtti)(*m_ui.m_changing_spec);
        m_copied_type = m_inserted_type;
    }
    if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_v && isKeyPressed(SDLK_LCTRL) && m_copied_spec)
    {
        m_copied_spec->pos = mouse_position;
        auto &new_obj = m_design_factories.at(m_copied_type)->create(*m_copied_spec);
        m_design.m_things[new_obj.getId()] = m_copied_spec;
    }

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
    {
        if (posIsEmpty(mouse_position) && m_ui.m_changing_spec) //! add to wall list
        {
            auto spec = m_ui.getSelectedSpec();
            spec->pos = grid_center_pos;
            spec->size = m_grid_cell_size;

            auto &new_wall = m_design_factories.at(m_inserted_type)->create(*spec);
            m_design.m_things[new_wall.getId()] = spec;
            m_ui.m_changing_spec = spec;
        }
    }

    if (event.type == SDL_MOUSEMOTION)
    {
        if (m_moved_point_id != -1)
        {
            m_design.m_named_positions.at(m_moved_point_id).coords = getGridSnappedPos(mouse_position, m_grid_cell_size, m_snap_mode);
        }
    }
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
    {
        // origin is clicked
        for (auto &[id, point] : m_design.m_named_positions)
        {
            if (utils::dist(point.coords, mouse_position) < 5.f)
            {
                m_moved_point_id = id;
                return;
            }
        }

        // m_selected_entity_ids.clear();
        for (auto p_obj : m_design_world->getEntities().data())
        {
            if (getBoundingRect(*p_obj).contains(mouse_position))
            {
                m_inserted_type = p_obj->getType();
                m_selected_entity_ids.clear();
                m_selected_entity_ids.insert(p_obj->getId());
                m_ui.m_changing_spec = m_design.m_things[p_obj->getId()];
                synchonizeSpec(m_ui.m_changing_spec, *p_obj);
                m_on_spec_change_cb = [p_obj, this](std::shared_ptr<GameObjectSpec> new_spec) { //! remove and create new object with new spec data
                    if (!m_selected_entity_ids.empty())
                    {
                        int selected_ent_id = *m_selected_entity_ids.begin();
                        auto p_selected_obj = m_design_world->get(selected_ent_id);
                        p_selected_obj->kill();
                        auto &new_obj = m_design_factories.at(p_selected_obj->getType())->create(*new_spec);
                        m_design.m_things.erase(selected_ent_id);
                        m_design.m_things[new_obj.getId()] = new_spec;
                        m_selected_entity_ids.erase(selected_ent_id);
                        m_selected_entity_ids.insert(new_obj.getId());
                    }
                };
                m_drag_pos_start = mouse_position;
                m_old_obj_pos = p_obj->getPosition();
                m_is_dragging = true;
            }
        }
        for (auto &[id, arrow] : m_size_arrows)
        {
            arrow->handlePushEvent(mouse_position);
        }
    }
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
    {
        if (m_moved_point_id != -1)
        {
            m_moved_point_id = -1; //! reset moving point
        }
        if (m_is_dragging && m_snap_mode != SnapMode::None && !m_selected_entity_ids.empty())
        {
            auto p_obj = m_design_world->get(*m_selected_entity_ids.begin());
            auto new_pos = m_old_obj_pos + (m_window.getMouseInWorld() - m_drag_pos_start);
            p_obj->setPosition(getGridSnappedPos(mouse_position, m_grid_cell_size, m_snap_mode));
            synchonizeSpec(m_ui.m_changing_spec, *p_obj);
        }
        m_is_dragging = false;
        for (auto &[id, arrow] : m_size_arrows)
        {
            arrow->handleReleaseEvent(mouse_position);
        }
    }

    m_window.m_view = old_view;
}

void Editor::handleCameraMotion(const SDL_Event &event)
{

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_MIDDLE)
    {
        m_dragging_camera = true;
        m_camera_drag_start_pos = m_camera.getView().getCenter();
        utils::Vector2f mouse_pos_sc = m_window.getMouseInScreen();
        utils::Vector2f window_size = m_window.getTargetSize();
        m_camera_drag_start_mouse_gl = glm::vec4(2. * mouse_pos_sc.x / window_size.x - 1, -2. * mouse_pos_sc.y / window_size.y + 1, 0, 1);
    }
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_MIDDLE)
    {
        m_dragging_camera = false;
    }

    if (event.type == SDL_KEYDOWN)
    {
    }
    if (event.type == SDL_KEYUP)
    {
        if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT)
        {
            m_camera_vel.x = 0.f;
        }
        if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN)
        {
            m_camera_vel.y = 0.f;
        }
    }
    if (event.type == SDL_MOUSEWHEEL)
    {
        if (event.wheel.y > 0)
        {
            m_camera.zoom(0.9f);
        }
        else if (event.wheel.y < 0)
        {
            m_camera.zoom(1. / 0.9f);
        }
    }
}
std::vector<utils::Vector2f> generatePath(utils::Vector2f start_point)
{
    float min_length = 10.f;
    float max_length = 50.f;
    float max_angle = 90;
    float min_angle = -90;
    float max_d_angle = 30;

    int path_points = 40;

    std::vector<utils::Vector2f> path;
    path.push_back(start_point);
    float angle = 0.f;
    int dir = 1;
    for (int path_i = 1; path_i < path_points; ++path_i)
    {

        angle += dir * randf(2.f, max_d_angle);
        if (angle >= max_angle)
        {
            angle = max_angle;
            dir = -1;
        }
        if (angle <= min_angle)
        {
            angle = min_angle;
            dir = 1;
        }
        auto new_dir = utils::angle2dir(angle);
        auto new_point = path.back() + new_dir * randf(min_length, max_length);
        path.push_back(new_point);
    }
    return path;
}

Track expandPath(std::vector<utils::Vector2f> path)
{

    Track track;
    track.path = path;
    if (path.size() <= 3)
    {
        return track;
    }

    //! create right points;
    auto get_intersection = [](utils::Vector2f s1, utils::Vector2f t1, utils::Vector2f s2, utils::Vector2f t2)
    {
        auto dr = s2 - s1;
        Vec2 n1 = {t1.y, -t1.x};
        Vec2 n2 = {t2.y, -t2.x};
        auto dr_n1 = utils::dot(dr, n1);
        auto dr_n2 = utils::dot(dr, n2);
        float beta = dr_n1 / (utils::dot(t2, n1));
        return s2 - beta * t2;
    };

    auto get_dist = [](utils::Vector2f s1, utils::Vector2f t1, utils::Vector2f query)
    {
        auto dr = query - s1;
        float dr_dt = utils::dot(dr, t1);
        return utils::cross(dr, t1);
    };

    int orient = 1;
    float width = 50.f;
    utils::Vector2f right_dir = track.path.at(1) - track.path.at(0);
    right_dir /= utils::norm(right_dir);
    utils::Vector2f right_start = path.at(0) + width * Vec2(right_dir.y, -right_dir.x);
    utils::Vector2f t1 = path.at(2) - path.at(1);
    t1 /= utils::norm(t1);
    utils::Vector2f s1 = path.at(1) + width * Vec2{t1.y, -t1.x};
    auto inters = get_intersection(right_start, right_dir, s1, t1);
    track.right_border.push_back(right_start);
    track.right_border.push_back(inters);

    auto points_count = path.size();
    for (int i = 2; i < points_count - 1; ++i)
    {
        utils::Vector2f next = path.at(i + 1);
        utils::Vector2f curr = path.at(i);
        utils::Vector2f t2 = next - curr;
        t2 /= utils::norm(t2);
        utils::Vector2f s2 = curr + width * Vec2{t2.y, -t2.x};

        bool turns_right = utils::cross(t2, t1) > 0.f;
        float dist_to_next = get_dist(curr, t2, inters);
        if (turns_right && dist_to_next > 0.f && dist_to_next < width)
        {
            // while(dist_to_next > 0.f  && dist_to_next < width)
            {
                int last = track.right_border.size();
                right_start = track.right_border.at(last - 2);
                right_dir = track.right_border.at(last - 1) - right_start;
                right_dir /= utils::norm(right_dir);
                inters = get_intersection(right_start, right_dir, s2, t2);

                dist_to_next = get_dist(curr, t2, inters);
                track.right_border.pop_back();
            }
            track.right_border.push_back(inters);
            s1 = inters;
            t1 = t2;
        }
        else
        {
            inters = get_intersection(s1, t1, s2, t2);
            track.right_border.push_back(inters);
            right_start = s1;
            right_dir = t1;
            s1 = s2;
            t1 = t2;
        }
    }

    auto right_end = path.back() + width * Vec2{t1.y, -t1.x};
    track.right_border.push_back(right_end);

    return track;
}

void drawPath2(const std::vector<utils::Vector2f> &path, Renderer &window)
{
    for (int i = 0; i < path.size(); ++i)
    {
        window.drawCricleBatched(path.at(i), 10, {1, 0, 1, 1});
    }
    for (int i = 1; i < path.size(); ++i)
    {
        auto curr = path.at(i - 1);
        auto next = path.at(i);

        window.drawLineBatched(curr, next, 2, {0, 1, 0, 1});
    }
}

void drawTrack(const Track &track, Renderer &window)
{
    drawPath2(track.path, window);
    drawPath2(track.right_border, window);
}

void Editor::handleEvent(const SDL_Event &event)
{
    auto mouse_position = m_window.getMouseInWorld();

    //! imgui handled the event so we should not
    if (m_ui.handleEvent(event))
    {
        return;
    }

    handleCameraMotion(event);

    /*     if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
        {
            auto pos = m_snap_to_grid ? getGridSnappedPos(mouse_position, m_grid_cell_size) : mouse_position;
            track.path.push_back(pos);
            return;
        }

        if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_g)
        {
            track.path = generatePath(mouse_position);
        }
        if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_d)
        {
            track.path.clear();
            track.left_border.clear();
            track.right_border.clear();
        }
        if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_r)
        {
            track = expandPath(track.path);
            return;
            }*/
    if (m_selecting_path)
    {
        handleEventPath(event);
    }
    else
    {
        handleEventDesigner(event);
    }

    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
    {
        int new_width = event.window.data1;
        int new_height = event.window.data2;
        // m_layers.resize(new_width, new_height);
        std::cout << "Changed window size in game!" << m_window.getTargetSize().x << " " << m_window.getTargetSize().y << std::endl;
        m_camera.setSize({800.f * (float)(new_width) / new_height, 800.f});

        m_layers.resize(new_width, new_height);
        initializeUI();
    }
}

//! \brief parse events and normal input
void Editor::parseInput(Renderer &window, float dt)
{
    if (isKeyPressed(SDLK_LEFT))
    {
    }
}

void Editor::update(const float dt)
{

    m_camera.setPostition(m_camera.getView().getCenter() + m_camera_vel * dt);

    m_design_world->update(dt * (m_move_objects));

    // m_world->update(0);
    m_timers.update(0);

    messanger.distributeMessages();
};

void Editor::synchonizeSpec(std::shared_ptr<GameObjectSpec> spec, GameObject &obj)
{
    spec->pos = obj.getPosition();
    spec->size = obj.getSize();
    spec->angle = obj.getAngle();
    spec->max_speed = obj.m_max_vel;
    spec->max_acc = obj.m_max_acc;
    spec->vel = obj.m_vel;
}

void Editor::drawSizeArrows(Renderer &window)
{
    auto old_view = window.m_view;
    // window.m_view = window.getDefaultView();

    if (!m_selected_entity_ids.empty())
    {
        auto selected_id = *m_selected_entity_ids.begin();
        auto &selected_obj = *m_design_world->get(selected_id);
        auto pos = selected_obj.getPosition();
        auto size = selected_obj.getSize();
        for (auto &[id, arrow] : m_size_arrows)
        {
            auto dr = m_arrows_dr.at(id);
            arrow->setPosition(pos.x + dr.x * (size.x / 2.f + 10.f), pos.y + dr.y * (size.y / 2.f + 10.f));
            if (arrow->isPushed())
            {
                auto delta = (window.getMouseInWorld() - m_size_change_start_pos);
                utils::Vector2f delta_r = {delta.x * dr.x, delta.y * dr.y};
                auto new_size = size + delta_r;
                auto new_pos = pos + delta / 2.f;
                m_size_change_start_pos = window.getMouseInWorld();
                selected_obj.setSize(new_size);
                selected_obj.setPosition(new_pos);
                synchonizeSpec(m_ui.m_changing_spec, selected_obj);
            }
            arrow->draw(window);
        }
    }
    window.m_view = old_view;
}

void Editor::drawDesigner(Renderer &window)
{
    if (m_selecting_path)
    {
        drawPath(window);
    }

    auto old_view = window.m_view;
    // window.m_view = window.getDefaultView();
    auto view_size = window.m_view.getSize();
    if (m_is_dragging)
    {
        for (auto &ent_id : m_selected_entity_ids)
        {
            auto p_obj = m_design_world->get(ent_id);
            auto pos = p_obj->getPosition();
            auto new_pos = m_old_obj_pos + (m_window.getMouseInWorld() - m_drag_pos_start);
            p_obj->setPosition(new_pos);
            synchonizeSpec(m_design.m_things.at(ent_id), *p_obj);
        }
    }

    auto mouse_position = m_window.getMouseInWorld();
    utils::Vector2f grid_center_pos = getGridSnappedPos(mouse_position, m_grid_cell_size);

    RectangleSimple mouse_rect({0, 1, 0, 0.3});
    mouse_rect.setPosition(grid_center_pos);
    mouse_rect.setScale(m_grid_cell_size);
    window.drawRectangle(mouse_rect);

    if (m_dragging_camera)
    {
        auto world2gl_mat = m_camera.getView().getMatrix();
        auto gl2world_mat = glm::inverse(world2gl_mat);
        utils::Vector2f mouse_pos_sc = m_window.getMouseInScreen();
        utils::Vector2f window_size = m_window.getTargetSize();
        auto mouse_pos_gl = glm::vec4(2. * mouse_pos_sc.x / window_size.x - 1.f, -2. * mouse_pos_sc.y / window_size.y + 1.f, 0, 1);
        auto delta_mouse_pos_gl = mouse_pos_gl - m_camera_drag_start_mouse_gl;
        auto delta_camera_pos = gl2world_mat * delta_mouse_pos_gl;
        utils::Vector2f delta_camera_pos2 = {delta_camera_pos.x, delta_camera_pos.y};
        m_camera.setPostition(m_camera_drag_start_pos - delta_camera_pos2);
    }

    drawGrid(window);
    drawLevelBox(window);

    for (auto ent_id : m_selected_entity_ids)
    {
        drawBoundingBox(*m_design_world->get(ent_id), window, {1, 0, 1, 1});
    }

    drawSizeArrows(window);
    window.drawAll();

    m_layers.clearAllLayers();
    m_layers.setView(m_window.m_view);
    m_design_world->draw(m_layers, m_assets, m_window, m_window.m_view);
    m_layers.drawInto(m_window);

    window.m_view = old_view;
}

void Editor::drawGrid(Renderer &window)
{
    auto view = window.m_view;
    utils::Vector2f left_view_pos = view.getCenter() - view.getScale();
    utils::Vector2f right_view_pos = view.getCenter() + view.getScale();
    utils::Vector2i first_grid_cells = {std::floor(left_view_pos.x / m_grid_cell_size.x), std::floor(left_view_pos.y / m_grid_cell_size.y)};

    utils::Vector2f grid_pos_vert = {first_grid_cells.x * m_grid_cell_size.x, left_view_pos.y};
    utils::Vector2f grid_pos_horiz = {left_view_pos.x, first_grid_cells.y * m_grid_cell_size.y};
    while (grid_pos_vert.x < right_view_pos.x)
    {
        window.drawLineBatched(grid_pos_vert, {grid_pos_vert.x, right_view_pos.y}, 4, {0, 1, 0, 1});
        grid_pos_vert.x += m_grid_cell_size.x;
    }
    while (grid_pos_horiz.y < right_view_pos.y)
    {
        window.drawLineBatched(grid_pos_horiz, {right_view_pos.x, grid_pos_horiz.y}, 4, {0, 1, 0, 1});
        grid_pos_horiz.y += m_grid_cell_size.y;
    }
}

void Editor::draw(Renderer &window)
{
    window.m_view = m_camera.getView();
    auto old_view = window.m_view;
    drawDesigner(window);
    drawTrack(track, window);
    if (m_draw_bounding_boxes)
    {
        drawBoundingBoxes(window);
    }
    window.drawAll();
    //! draw named points;
    for (auto &[id, point] : m_design.m_named_positions)
    {
        if (id == m_moved_point_id)
        {
            window.drawCricleBatched(m_design.m_named_positions.at(id).coords, 10, {0.1, 1, 0.1, 1});
        }
        else
        {
            window.drawCricleBatched(point.coords, 5, {0, 1, 1, 1});
        }
    }
    window.drawAll();

    //! draw the scene into the window
    // m_window.m_view = m_window.getDefaultView();
    for (auto &[id, button] : m_buttons)
    {
        button->draw(m_window);
    }
    auto window_size = m_window.getTargetSize();

    auto old_factors = m_window.m_blend_factors;

    m_window.m_view = old_view;
    m_window.m_blend_factors = old_factors;

    m_ui.draw();
}

void Editor::registerSystems()
{
    auto &systems = m_design_world->m_systems;

    systems.registerSystem(std::make_shared<TimedEventSystem>(systems.getComponents<TimedEventComponent>()));
    systems.registerSystem(std::make_shared<SpriteSystem>(systems.getComponents<SpriteComponent>(), m_layers));
    systems.registerSystem(std::make_shared<PathSystem>(systems.getComponents<PathComponent>(), m_layers));

    std::filesystem::path animation_directory = {
        std::string{RESOURCES_DIR} + "Textures/Animations/"};
    //    animation_directory /= ;
    auto animation_system = std::make_shared<AnimationSystem>(
        systems.getComponents<AnimationComponent>(),
        animation_directory, animation_directory);
    animation_system->registerAnimation(m_atlases.get("PurpleExplosion"), "PurpleExplosion");
    animation_system->registerAnimation(m_atlases.get("BlueExplosion"), "BlueExplosion");

    systems.registerSystem(animation_system);
}

bool Editor::isKeyPressed(SDL_Keycode key)
{
    auto *keystate = SDL_GetKeyboardState(NULL);
    return keystate[SDL_GetScancodeFromKey(key)];
}

void Editor::addDesignToWorld(utils::Vector2f pos, const nlohmann::json &data)
{
    auto &wall_data = data["objects"];
    for (auto &object_data : wall_data)
    {
        /* if (obj_type.has_value())
        {
            auto spec = deserializeSpec(obj_type.value(), object_data);
            spec->pos += pos;
            auto &new_obj = m_design_factories.at(ObjectType::Wall)->create(*spec);
        } */
    }
}

Editor::DesignedThing Editor::designFromJson(nlohmann::json &data)
{
    Editor::DesignedThing design;

    design.m_things.clear();

    if (data.contains("named_points"))
    {
        auto points_data = data.at("named_points");
        int id = 0;
        for (auto &[name, coords] : points_data.items())
        {
            DesignedThing::NamedPoint new_point = {Vec2{coords[0], coords[1]}, name};
            design.m_named_positions[id] = new_point;
            id++;
        }
    }
    else
    {
        fromJson(design.m_origin, "origin", data);
    }

    fromJson(design.m_level_size, "level_size", data);
    for (auto &spec_json : data.at("objects"))
    {
        std::string type_name_key = spec_json.at("obj_type");
        try
        {
            auto type_id = stringToEnum<TypeId>(type_name_key);
            auto spec = deserializeSpec(type_id, spec_json);
            auto &new_obj = m_design_factories.at(type_id)->create(*spec);
            design.m_things[new_obj.getId()] = spec;
        }
        catch (std::exception &e)
        {
            std::cout << "Enum does not exist: " << e.what() << std::endl;
        }
    }

    m_moved_point_id = -1;
    return design;
}

void serializeDesign(Editor::DesignedThing &design, std::string design_name, nlohmann::json &designs_data)
{
    if (designs_data.contains(design_name))
    {
        std::cout << "Cannot add new design because key: " << design_name << " exists!" << std::endl;
        return;
    }
    //! add new design into json
    designs_data[design_name] = {};
    auto &new_design = designs_data[design_name];
    new_design["named_points"] = {};
    for (auto &[id, point] : design.m_named_positions)
    {
        new_design.at("named_points")[point.name] = {point.coords.x, point.coords.y};
    }
    new_design["origin"] = {design.m_origin.x, design.m_origin.y};
    auto bb = design.getBoundingBox();
    new_design["bounds"] = {bb.pos_x, bb.pos_y, bb.width, bb.height};
    new_design["level_size"] = {design.m_level_size.x, design.m_level_size.y};

    std::vector<nlohmann::json> things = {};
    for (auto &[id, spec_p] : design.m_things)
    {
        auto type_name = std::string(enumToString(spec_p->obj_type));
        things.push_back(serializeSpec(*spec_p));
    }
    new_design["objects"] = things;
}

void Editor::drawBoundingBoxes(Renderer &window)
{
    for (auto &p_obj : m_design_world->getEntities().data())
    {
        drawBoundingBox(*p_obj, window, {0, 1, 1, 1});
    }
}

void Editor::drawLevelBox(Renderer &window)
{
    auto center = m_design.m_origin + 0.5f * m_design.m_level_size;
    drawBoundingBox(center, m_design.m_level_size / 2.f, window, {1, 1, 0, 1}, 4.f);
}

template <class... TypeList>
void Editor::createFactories()
{
    ((m_designable_types.insert(getTypeId<TypeList>())), ...);
    ((m_design_factories[getTypeId<TypeList>()] = std::make_unique<EntityFactory2<TypeList>>(*m_design_world)), ...);
}