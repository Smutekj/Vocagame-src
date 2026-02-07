#include "IntroScreen.h"

#include "Menu/UIButtons.h"
#include "Animation.h"
#include "JSBindings.h"


IntroScreen::IntroScreen(Renderer &window, Assets &assets, const nlohmann::json &text_resources, std::function<void()> start_callback)
    : m_window(window),
      m_textures(assets.textures),
      m_font(assets.fonts.begin()->second),
      m_text_resources(text_resources),
      m_start_button("Start", *m_font)
{
    m_intro.setFont(assets.fonts.at("Tahoma").get());
    m_intro.setPadding({10, 10});
    std::string intro_text = text_resources.value("Intro", "INTRO TEXT MISSING");
    intro_text += "\n" + text_resources.value("ControlsPC", "CONTROLS TEXT MISSING");
    m_intro.setText(intro_text);

    m_intro_text_pos = {m_window.getTargetSize().x / 4.f, -m_window.getTargetSize().y * 2. / 10.};
    utils::Vector2f m_intro_text_final_pos = {m_intro_text_pos.x, m_window.getTargetSize().y * 3. / 4.f};
    positionAnimation(1.4f, 0.f, m_intro_text_pos, m_intro_text_final_pos, m_intro_text_pos, m_timers);
    m_start_button.setOnClick([this, m_intro_text_final_pos, start_callback]()
                              { positionAnimation(1.0f, 0.f,
                                                      m_intro_text_final_pos,
                                                      {m_intro_text_pos.x, -m_window.getTargetSize().y * 2.f / 10.f}, m_intro_text_pos,
                                                      m_timers);
                                                    
                                    m_timers.runDelayed(start_callback, 1.0f); });

    m_start_button.setScale(80.f, 30.f);
    m_start_button.m_text.setColor({25, 255, 25, 255});
    m_start_button.m_text.m_edge_color = m_start_button.m_text.m_color;
    m_start_button.m_on_hover_stop = [](TextButton &button)
    {
        button.m_text.m_edge_color = {0, 255, 25, 255};
    };
    m_start_button.setOnHover([](TextButton &button)
                              { button.m_text.m_edge_color = {255, 25, 25, 255}; });

    auto width = m_window.getTargetSize().x;
    auto height = m_window.getTargetSize().y;

    TextureOptions options;
    options.wrap_x = TexWrapParam::ClampEdge;
    options.wrap_y = TexWrapParam::ClampEdge;
    options.mag_param = TexMappingParam::Linear;
    options.min_param = TexMappingParam::Linear;

    float dpr = js::getDpr();
    auto &bg_layer = m_layers.addLayer("Background", 1, options, width, height, 1);
    bg_layer.m_canvas.addShader("gradient2", "basictex.vert", "gradient2.frag");
    bg_layer.m_canvas.addShader("gradientCircle", "basictex.vert", "gradientCircle.frag");

    auto &text_layer = m_layers.addLayer("Text", 3, options, width, height, 1);
    dpr = std::min(dpr, 2.f);
    auto &bloom_layer = m_layers.addLayer("Bloom", 0, options, width / dpr, height / dpr, dpr);
    bloom_layer.setBackground({0, 0, 0, 0});
    std::filesystem::path shaders_directory = std::string{RESOURCES_DIR} + "Shaders/";
    bloom_layer.m_canvas.setShadersPath(shaders_directory);
    bloom_layer.m_canvas.addShader("gradientX", "basictex.vert", "gradientX.frag");
    bloom_layer.m_canvas.addShader("gradientY", "basictex.vert", "gradientY.frag");
    bloom_layer.m_canvas.addShader("gradientSquare", "basictex.vert", "gradientY.frag");
    bloom_layer.m_canvas.addShader("textOutline", "basictex.vert", "textOutline.frag");
    bloom_layer.addEffect(std::make_unique<BloomPhysical>(width / dpr, height / dpr, 2, 2, options));

    window.addShader("gradient", "basictex.vert", "gradient.frag");
    window.addShader("gradient2", "basictex.vert", "gradient2.frag");
}

void IntroScreen::handleEvent(const SDL_Event &event)
{
    auto mouse_pos = m_window.getMouseInWorld();
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
    {
        m_start_button.handlePushEvent(mouse_pos);
    }
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
    {
        m_start_button.handleReleaseEvent(mouse_pos);
    }
}
void IntroScreen::update(float dt)
{
    m_timers.update(dt);
}

void IntroScreen::draw(Renderer &window)
{
    window.m_view = window.getDefaultView();
    auto win_size = window.getTargetSize();

    //! draw grey background;
    RectangleSimple r({0.1, 0.1, 0.15, 0.8});
    r.setPosition(win_size / 2.f);
    r.setScale(win_size);
    window.drawRectangle(r);
    window.drawAll();

    m_start_button.setPosition(win_size.x / 2.f, m_intro_text_pos.y - m_intro.getPageHeight() - 30.f);
    m_start_button.draw(m_layers, window);

    m_intro.setPageWidth(win_size.x / 2.f);
    m_intro.setPosition(m_intro_text_pos);
    utils::Vector2f page_size = {m_intro.getPageWidth(), m_intro.getPageHeight()};
    utils::Vector2f page_center = m_intro_text_pos + Vec2{page_size.x, -page_size.y} / 2.f;
    // intro.setScale(0.8f);

    m_layers.clearAllLayers();
    m_layers.setView(window.m_view);
    drawShinyRectangle(m_layers.getCanvas("Bloom"), page_center, page_size, {50, 5., 0., 1.});
    m_window.m_blend_factors = {BlendFactor::One, BlendFactor::OneMinusSrcAlpha};

    //! text background
    r.setScale(page_size);
    r.setPosition(page_center);
    r.m_color = {6 / 255., 2 / 255., 64 / 255., 1};
    m_layers.getCanvas("Background").drawRectangle(r, "gradient2");
    m_intro.drawInto(m_layers.getCanvas("Text"));
    m_layers.drawInto(window);
};

MainMenu::MainMenu(Renderer &window, Assets &assets, const nlohmann::json &text_resources,
                   std::function<void(std::string)> start_callback)
    : m_window(window)
{
    auto font = assets.fonts.begin()->second;

    for (const auto &[game_name, texts] : text_resources.items())
    {
        auto m_start_g1 = std::make_unique<TextButton>(game_name, *font);
        m_start_g1->setScale(80.f, 30.f);
        m_start_g1->m_text.setColor({25, 255, 25, 255});
        m_start_g1->m_text.m_edge_color = {25, 255, 25, 255};
         m_start_g1->m_on_hover_stop = [](TextButton &button)
        {
            button.m_text.m_color = {25, 255, 25, 255};
        };
        m_start_g1->setOnHover([](TextButton &button)
                               { button.m_text.m_color = {255, 25, 25, 255}; });

        m_start_g1->setOnClick([=]()
                               { start_callback(game_name); });
        m_buttons.push_back(std::move(m_start_g1));
    }
}

void MainMenu::handleEvent(const SDL_Event &event)
{
    auto mouse_pos = m_window.getMouseInWorld();
    for (auto &button : m_buttons)
    {
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
        {
            button->handlePushEvent(mouse_pos);
        }
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
        {
            button->handleReleaseEvent(mouse_pos);
        }
    }
}
void MainMenu::update(float dt)
{
    m_timers.update(dt);
}

void drawCentered(std::vector<std::unique_ptr<UIButtonI>> &buttons,
                  float y_pos, float margin, Renderer &window)
{
    auto win_size = window.getTargetSize();
    //! find total size
    float total_height = std::accumulate(buttons.begin(), buttons.end(), 0.f, [&](float height, auto &button_p)
                                         { return height + button_p->getScale().y * 2.f; });
    total_height += (buttons.size() - 1) * margin;

    float x_pos = win_size.x / 2.f;
    for (auto &button_p : buttons)
    {
        button_p->setPosition(x_pos, y_pos);
        button_p->draw(window);
        y_pos += button_p->getScale().y * 2.f + margin;
    }
}

void MainMenu::draw(Renderer &window)
{
    window.m_view = window.getDefaultView();
    auto win_size = window.getTargetSize();

    //! draw grey background;
    RectangleSimple r({0.1, 0.1, 0.15, 0.8});
    r.setPosition(win_size / 2.f);
    r.setScale(win_size);
    window.drawRectangle(r);
    window.drawAll();

    drawCentered(m_buttons, win_size.y / 2.f, 10.f, window);
    window.drawAll();

    // m_intro.setPageWidth(win_size.x / 2.f);
    // m_intro.setPosition(m_intro_text_pos);
    // utils::Vector2f page_size = {m_intro.getPageWidth(), m_intro.getPageHeight()};
    // utils::Vector2f page_center = m_intro_text_pos + Vec2{page_size.x, -page_size.y} / 2.f;
    // // intro.setScale(0.8f);

    // m_layers.clearAllLayers();
    // m_layers.setView(window.m_view);
    // drawShinyRectangle(m_layers.getCanvas("Bloom"), page_center, page_size, {50, 5., 0., 1.});
    // m_window.m_blend_factors = {BlendFactor::One, BlendFactor::OneMinusSrcAlpha};

    // //! text background
    // r.setScale(page_size);
    // r.setPosition(page_center);
    // r.m_color = {6 / 255., 2 / 255., 64 / 255., 1};
    // m_layers.getCanvas("Background").drawRectangle(r, "gradient2");
    // m_intro.drawInto(m_layers.getCanvas("Text"));
    // m_layers.drawInto(window);
}