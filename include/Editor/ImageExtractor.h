
#pragma once

#include "DrawLayer.h"
#include "Window.h"
#include "Utils/Grid.h"

#include "Camera.h"
#include "TextureAtlas.h"
#include "Screen.h"

#include "PostOffice.h"
#include "PostBox.h"

#include "Commands.h"
#include "Assets.h"

namespace Editing
{

    class ImageExtractor : public Screen
    {

        PostOffice messanger;

    public:
        ImageExtractor(Renderer &window, KeyBindings &bindings, Assets &assets);
        virtual ~ImageExtractor() override;

        virtual void handleEvent(const SDL_Event &event) final;
        virtual void update(float dt) final {};
        virtual void draw(Renderer &window) final;

        void handleCameraMotion(const SDL_Event &event);
        // void handleCameraMotion(const SDL_Event &event);
        // void drawGrid(Renderer &window);
        void initializeUI();
        void drawUI();

        Camera m_camera;
        bool m_dragging_camera = false;
        utils::Vector2f m_camera_drag_start_pos;
        glm::vec4 m_camera_drag_start_mouse_gl;

        Renderer &m_window;

        TextureHolder &m_textures;
        TextureHolder2 &m_atlases;
        std::shared_ptr<Font> m_font;

        LayersHolder m_layers;

        std::unique_ptr<FrameBuffer> m_selected_tex_pixels;
        std::unique_ptr<Renderer> m_selected_tex_canvas;

        float m_save_file_scale = 0.5f;
        std::filesystem::path m_save_dir_path = "../Resources/TexEditorDir";
        std::string m_savefile_name;
        bool m_savefile_popup = false;
        bool m_text_focused = true;
        bool m_file_exists_warning_popup = false;

        bool m_selecting_texture = false;
        std::string m_selected_tex_id = "Arrow";
        Rectf m_texrect_selection;
        std::array<Vec2, 4> m_selection_corners;
        int m_selected_corner_id = -1;

        //! grid stuff
        utils::Vector2f m_grid_cell_size = {100, 100};
        enum class SnapMode
        {
            Center,
            Grid,
            None,
        };
        SnapMode m_snap_mode = SnapMode::Center;
        bool m_snap_to_grid = true;
    };

}; // namespace Editing