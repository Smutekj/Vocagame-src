#include "ImageExtractor.h"

#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <imgui_stdlib.h>

#include <filesystem>

namespace Editing
{

    template <class ContainerType, class KeyT, class ValueT>
    bool drawUISelection(ContainerType &container,
                         KeyT selection_name,
                         KeyT &selected_key,
                         std::function<void(ValueT &)> on_selection)
    {
        bool value_changed = false;
        if (ImGui::BeginListBox(selection_name.c_str()))
        {
            for (auto &[key, value] : container)
            {
                bool selected = (key == selected_key);
                if (ImGui::Selectable(key.c_str(), &selected))
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

    void refreshTexturesFromDir(TextureHolder &textures, std::filesystem::path directory_path)
    {
        namespace fs = ::std::filesystem;

        if (!fs::exists(directory_path) || !fs::is_directory(directory_path))
        {
            std::cout << "Cannot read animations in: " << directory_path.string() << " Path does not exist or is not directory!" << std::endl;
            return;
        }

        std::cout << "Reading Textures in path: " << directory_path << std::endl;
        for (auto const &dir_entry : fs::directory_iterator{directory_path})
        {
            auto entry_path = fs::path(dir_entry);
            if (entry_path.extension() == ".png")
            {
                std::string tex_id = entry_path.filename().replace_extension("");
                textures.erase(tex_id);
                textures.add(tex_id, entry_path);
            }
        }
    }

    ImageExtractor::~ImageExtractor()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
    }

    bool drawToFile(Rectf selection, Texture &selected_tex, std::string filename, std::filesystem::path dir, float scale = 1.f, bool forced = false)
    {
        if (std::filesystem::exists(dir / filename) && !forced)
        {
            return false;
        }
        Recti tex_rect = {(int)selection.pos_x,
                          (int)(selected_tex.getSize().y - selection.pos_y - selection.height),
                          (int)selection.width,
                          (int)selection.height};
        TextureOptions opt;
        opt.data_type = TextureDataTypes::UByte;
        opt.internal_format = TextureFormat::RGBA;
        opt.format = TextureFormat::RGBA;
        // opt.min_param= TexMappingParam::Linear;
        // opt.mag_param= TexMappingParam::Linear;
        auto pix = std::make_unique<FrameBuffer>(tex_rect.width * scale, tex_rect.height * scale, opt);
        Renderer canvas(*pix);

        Sprite s(selected_tex);
        s.m_tex_rect = tex_rect;
        std::cout << tex_rect.pos_x << " " << tex_rect.pos_y << std::endl;
        s.setPosition(Vec2{pix->getSize()} / 2.f);
        s.setScale(Vec2{pix->getSize()} / 2.f);
        canvas.m_view = canvas.getDefaultView();
        canvas.m_view.setSize(canvas.m_view.getSize().x, -canvas.m_view.getSize().y);
        canvas.drawSprite(s);
        canvas.drawAll();

        writeTextureToFile(dir, filename, *pix);
        return true;
    }

    void ImageExtractor::initializeUI()
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
        auto &window = static_cast<Window &>(m_window.getTarget());
        ImGui_ImplSDL2_InitForOpenGL(window.getHandle(), window.getContext());
        ImGui_ImplOpenGL3_Init(glsl_version);
    }

    ImageExtractor::ImageExtractor(Renderer &window, KeyBindings &bindings, Assets &assets)
        : m_window(window),
          m_textures(assets.textures),
          m_font(assets.fonts.begin()->second),
          m_atlases(assets.atlases),
          m_camera({400, 300}, {800, 800 * window.getTargetSize().y / window.getTargetSize().x}, messanger)
    {

        initializeUI();

        std::filesystem::path work_tex_dir = std::string{RESOURCES_DIR} + "TexEditorDir/";
        refreshTexturesFromDir(m_textures, work_tex_dir);

        auto &selected_tex = *m_textures.get(m_selected_tex_id);
        TextureOptions options;
        m_texrect_selection = {100, 100, 100, 200};
        options.data_type = TextureDataTypes::UByte;
        options.format = TextureFormat::RGBA;
        options.internal_format = TextureFormat::RGBA;
        m_selected_tex_pixels = std::make_unique<FrameBuffer>(1, 1, options);
    }

    Rectf getSelectionRect(std::array<Vec2, 4> corners)
    {
        float min_x = std::min_element(begin(corners), end(corners), [](auto &v1, auto &v2)
                                       { return v1.x < v2.x; })
                          ->x;
        float min_y = std::min_element(begin(corners), end(corners), [](auto &v1, auto &v2)
                                       { return v1.y < v2.y; })
                          ->y;
        float max_x = std::max_element(begin(corners), end(corners), [](auto &v1, auto &v2)
                                       { return v1.x < v2.x; })
                          ->x;
        float max_y = std::max_element(begin(corners), end(corners), [](auto &v1, auto &v2)
                                       { return v1.y < v2.y; })
                          ->y;
        return Rectf{min_x, min_y, max_x - min_x, max_y - min_y};
    }

    std::array<Vec2, 4> getSelectionCorners(Rectf rect)
    {
        return {Vec2{rect.pos_x, rect.pos_y},
                Vec2{rect.pos_x + rect.width, rect.pos_y},
                Vec2{rect.pos_x + rect.width, rect.pos_y + rect.height},
                Vec2{rect.pos_x, rect.pos_y + rect.height}};
    }

    bool isOnLine(Vec2 a, Vec2 b, Vec2 query, float dist)
    {
        Vec2 dt = b - a;
        Vec2 dq = query - a;
        Vec2 n = {dt.y, -dt.x};

        float l = utils::dist(a, b);
        float dq_dt = utils::dot(dt, dq) / l;
        float norm_dist = std::abs(utils::dot(dq, n) / l);
        bool within_l = dq_dt > 0.f && dq_dt <= l &&
                        norm_dist <= dist;
        return within_l;
    }

    void ImageExtractor::handleCameraMotion(const SDL_Event &event)
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
        if (m_dragging_camera && event.type == SDL_MOUSEMOTION)
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

        if (event.type == SDL_KEYDOWN)
        {
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

    void ImageExtractor::handleEvent(const SDL_Event &event)
    {
        //! imgui event handling
        ImGui_ImplSDL2_ProcessEvent(&event);
        ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureKeyboard && event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
        {
            return;
        }
        if (io.WantCaptureMouse)
        {
            return;
        }

        handleCameraMotion(event);

        //! our events
        m_selection_corners = getSelectionCorners(m_texrect_selection);
        auto mouse_pos = m_window.getMouseInWorld();
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
        {
            m_selected_corner_id = -1;
        }
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
        {
            int corner_id = 0;
            for (auto &p : m_selection_corners)
            {
                if (utils::dist(p, mouse_pos) < 10.f)
                {
                    m_selected_corner_id = corner_id;
                    std::cout << m_selected_corner_id << std::endl;
                    break;
                }
                corner_id++;
            }
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_r)
        {
            m_texrect_selection.pos_x = mouse_pos.x;
            m_texrect_selection.pos_y = mouse_pos.y;
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_s)
        {
            m_savefile_popup = true;
            m_text_focused = true;
        }
        if (event.type == SDL_MOUSEMOTION && m_selected_corner_id != -1)
        {
            auto &p = m_selection_corners.at(m_selected_corner_id);
            //! maintain orthogonality
            auto &next_p = m_selection_corners.at((m_selected_corner_id + 1) % 4);
            auto &prev_p = m_selection_corners.at((m_selected_corner_id + 4 - 1) % 4);
            auto delta_n = next_p - p;
            auto delta_p = p - prev_p;
            if (delta_n.x == 0.f)
            {
                next_p.x = mouse_pos.x;
            }
            else if (delta_n.y == 0.f)
            {
                next_p.y = mouse_pos.y;
            }
            if (delta_p.x == 0.f)
            {
                prev_p.x = mouse_pos.x;
            }
            else if (delta_p.y == 0.f)
            {
                prev_p.y = mouse_pos.y;
            }
            p = mouse_pos;

            m_texrect_selection = getSelectionRect(m_selection_corners);
        }
    }

    void drawBoundingBox(utils::Vector2f pos, utils::Vector2f scale, Renderer &window, Color color, float thickness = 2.f)
    {
        window.drawLineBatched({pos.x + scale.x, pos.y + scale.y}, {pos.x + scale.x, pos.y - scale.y}, thickness, color);
        window.drawLineBatched({pos.x + scale.x, pos.y - scale.y}, {pos.x - scale.x, pos.y - scale.y}, thickness, color);
        window.drawLineBatched({pos.x - scale.x, pos.y - scale.y}, {pos.x - scale.x, pos.y + scale.y}, thickness, color);
        window.drawLineBatched({pos.x - scale.x, pos.y + scale.y}, {pos.x + scale.x, pos.y + scale.y}, thickness, color);
    }

    void drawRectSelection(Rectf rect, Renderer &canvas)
    {
        Vec2 center = {rect.pos_x + rect.width / 2.f, rect.pos_y + rect.height / 2.f};
        Vec2 scale = {rect.width / 2.f, rect.height / 2.f};
        drawBoundingBox(center, scale, canvas, {0, 0, 1, 1}, 3);
    }

    void ImageExtractor::draw(Renderer &window)
    {
        window.m_view = m_camera.getView();
        auto old_view = window.m_view;

        auto selected_tex = m_textures.get(m_selected_tex_id);
        if (selected_tex)
        {
            Vec2 selected_tex_size = selected_tex->getSize();
            Sprite s(*selected_tex);
            s.setPosition(selected_tex_size / 2.f);
            s.setScale(selected_tex_size / 2.f);
            window.drawSprite(s);
        }

        drawRectSelection(m_texrect_selection, window);

        window.drawAll();

        m_window.m_view = old_view;
        window.m_view = old_view;

        drawUI();
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

    void ImageExtractor::drawUI()
    {
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Select Texture", &m_selecting_texture);
        std::string sel_id = "Texture";
        std::function<void(std::shared_ptr<Texture> &)> wtf = [](std::shared_ptr<Texture> &tex_p) {};
        drawUISelection(m_textures.getTextures(), sel_id, m_selected_tex_id, wtf);
        ImGui::End();

        //! position popups in the middle
        auto window_size = m_window.getTargetSize();
        Vec2 popup_size = {300, 100};
        ImGui::SetNextWindowPos(ImVec2(window_size.x / 2.f - popup_size.x / 2.f, window_size.y / 2.f));
        ImGui::SetNextWindowSize(ImVec2(popup_size.x, popup_size.y));

        if (m_file_exists_warning_popup)
        {
            ImGui::Begin("WARNING");

            auto confirm = [this]()
            {
                drawToFile(m_texrect_selection, *m_textures.get(m_selected_tex_id), m_savefile_name + ".png", "../", m_save_file_scale, true);
                m_savefile_popup = false;
                m_file_exists_warning_popup = false;
            };
            auto cancel = [this]()
            { m_file_exists_warning_popup = false; };
            std::string warning_text = "Image: " + m_savefile_name + " exists!";
            drawWarningPopup(warning_text, confirm, cancel);

            ImGui::End();
        }

        if (m_savefile_popup)
        {
            ImGui::Begin("Name saved file!");

            if (m_text_focused)
            {
                ImGui::SetKeyboardFocusHere(0);
                m_text_focused = false;
            }

            ImGui::InputText("Filename", &m_savefile_name, ImGuiInputTextFlags_EnterReturnsTrue);
            bool enter_pressed = ImGui::IsItemDeactivatedAfterEdit();

            ImGui::SameLine();
            ImGui::InputFloat("Scale", &m_save_file_scale);
            if (ImGui::Button("Save"))
            {
                bool save_success = drawToFile(m_texrect_selection, *m_textures.get(m_selected_tex_id), m_savefile_name + ".png", "../", m_save_file_scale);
                if (save_success)
                {
                    m_savefile_popup = false;
                    m_text_focused = false;
                    ImGui::SetWindowFocus(nullptr);
                }
                else
                {
                    m_file_exists_warning_popup = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                m_savefile_popup = false;
                m_text_focused = false;
                ImGui::SetWindowFocus(nullptr);
            }

            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
} // namespace Editing