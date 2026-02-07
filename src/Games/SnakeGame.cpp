#include "SnakeGame.h"

#include "Utils/RandomTools.h"

#include "Systems/SpriteSystem.h"
#include "Systems/TimedEventSystem.h"

bool game_started = false;
std::queue<std::string> message_queue;

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
EM_JS(void, sendControlMessage, (const char *msg), {
    Module.sendControlMessage(UTF8ToString(msg));
});
EM_JS(int, getClientId, (), {
    return Module["client_id"];
});
EM_JS(int, getServerTime, (), {
    return Module["server_time"];
});
extern "C"
{
    void receiveMessage(const char *msg)
    {
        message_queue.push(msg);
    }
};
#else
#include <boost/beast.hpp>

void sendControlMessage(const char *msg)
{
    std::cout << "Sending " << msg << std::endl;
}
void receiveMessage(const char *msg)
{
    message_queue.push(msg);
}
int getClientId()
{
    return 0;
}
#endif
TextRect &SnakeGame::generateWord2(bool is_correct, utils::Vector2f pos, utils::Vector2f size)
{
    TextRect::Spec spec;
    auto random_word_repre = m_generators.at(m_topics.at(0)).getNext();
    if (is_correct)
    {
        spec.text = random_word_repre.correct_form;
    }
    else
    {
        spec.text = utils::randomValue(m_group_repres.at(random_word_repre.group)).correct_form;
    }

    spec.pos = pos;
    spec.size = size;
    spec.meaning_id = random_word_repre.meaning_id;
    spec.translation = random_word_repre.translation;
    spec.correct_form = random_word_repre.correct_form;
    auto &word = static_cast<TextRect &>(m_world->createObject(spec));

    std::string correct_no_space = spec.meaning_id;
    if (std::size_t pos = spec.meaning_id.find(' '); pos != std::string::npos)
    {
        std::string correct_no_space = spec.meaning_id.erase(spec.meaning_id.find(' '), 1);
    }

    if (m_atlases.contains(correct_no_space))
    {
        word.setImage(m_atlases.getSprite(correct_no_space));
        word.setSize({size.x / 2.f, size.y});
    }
    return word;
};

SnakeGame::SnakeGame(Renderer &window, KeyBindings &bindings, Assets &assets)
    : Game(window, bindings, assets),
      m_pos_generator(messanger, {m_font->getLineHeight() * 10.f, m_font->getLineHeight() * 5.f}, {800.f, 800.f})
{
    m_camera.setPostition({400, 400});
    m_camera.setSize({800, 800});
    m_camera.m_can_move_x = false;
    m_camera.m_can_move_y = false;

    registerSystems();

    m_client_id = getClientId();
    // m_time_stamp = getServerTime();

    nlohmann::json start_msg;
    start_msg["type"] = "client-started";
    sendControlMessage(start_msg.dump().c_str());
}

void SnakeGame::registerSystems()
{
    auto &colllider = m_world->getCollisionSystem();

    auto &systems = m_world->m_systems;
    systems.registerSystem(std::make_shared<TransformSystem>(systems.getComponents<TransformComponent>()));
    systems.registerSystem(std::make_shared<TimedEventSystem>(systems.getComponents<TimedEventComponent>()));
    systems.registerSystem(std::make_shared<SpriteSystem>(systems.getComponents<SpriteComponent>(), m_layers));

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

namespace chr = std::chrono;
using clk = chr::steady_clock;
decltype(clk::now()) tic;

void SnakeGame::updateImpl(float dt)
{
    auto toc = clk::now();
    auto update_interval = chr::duration_cast<chr::microseconds>(toc - tic).count();
    m_time_stamp += update_interval;
    tic = toc;

    while (!message_queue.empty())
    {
        std::string control_message = message_queue.front();
        message_queue.pop();

        nlohmann::json msg_data = nlohmann::json::parse(control_message);
        if (msg_data.at("type") == "game-started")
        {
            m_time_stamp = msg_data.at("time").get<std::uint64_t>();
            std::cout << "Snake Created at time: " << m_time_stamp * 1e-6 << " s";
            auto create_snake = [this, msg_data](int ent_id)
            {
                Snake::Spec spec;
                spec.pos = {200, 200};
                spec.size = {50, 50};
                spec.time = m_time_stamp * 1e-6;
                return std::make_shared<Snake>(*m_world, spec, ent_id);
            };
            m_snake = std::static_pointer_cast<Snake>(m_world->insertObject(create_snake));
            game_started = true;
        }
        if (msg_data.at("type") == "object-created")
        {
            auto spec_data = msg_data.at("spec");
            if (spec_data.at("type") == static_cast<int>(TypeId::TextRect))
            {
                utils::Vector2f pos = {spec_data.at("pos")[0], spec_data.at("pos")[1]};
                utils::Vector2f size = {spec_data.at("size")[0], spec_data.at("size")[1]};
                bool correct = spec_data.at("correct");
                auto &new_word = generateWord2(correct, pos, size);
            }
        }
        if (msg_data.at("type") == "world")
        {
            const auto &objects = msg_data.at("objects");
            std::cout << "world MSG" << std::endl;
            for (const auto &[key, spec_data] : objects.items())
            {
                if (spec_data.value("type", 0) == static_cast<int>(TypeId::TextBubble) || static_cast<int>(TypeId::TextRect))
                {
                    utils::Vector2f pos = {spec_data.at("pos")[0], spec_data.at("pos")[1]};
                    utils::Vector2f size = {spec_data.at("size")[0], spec_data.at("size")[1]};
                    bool correct = spec_data.at("correct");
                    auto &new_word = generateWord2(correct, pos, size);
                }
            }
            nlohmann::json msg;
            msg["type"] = "client-ready";
            std::cout << "SENDING READY MSG!" << std::endl;
            sendControlMessage(msg.dump().c_str());
        }
        if (msg_data.at("type") == "snakes")
        {
            WorldState new_state;
            new_state.server_time = msg_data.at("time");

            const auto &objects = msg_data.at("objects");
            for (const auto &[key, spec_data] : objects.items())
            {
                int client_id = spec_data.value("client_id", 0);
                utils::Vector2f pos = {spec_data.at("pos")[0], spec_data.at("pos")[1]};
                float angle = spec_data.at("angle");
                int dir = spec_data.at("dir");
                if (client_id == m_client_id)
                {
                    // m_snake->synchronize(pos, angle, msg_data.at("time").get<std::uint64_t>() * 1e-6);
                }
                new_state.snakes.push_back(SnakeState{.ent_id = spec_data.at("id"), .pos = pos, .angle = angle, .dir = dir});
            }

            m_history.push_back(new_state);
        }
    }

    auto interpolateState = [](WorldState w1, WorldState w2, std::int64_t t)
    {
        double dt = (double)w1.server_time - (double)t;
        double dt_st = (double)w2.server_time - (double)w1.server_time;
        WorldState interp_state;
        for (int i = 0; i < w1.snakes.size(); ++i)
        {
            auto s1 = w1.snakes.at(i);
            auto s2 = w2.snakes.at(i);
            SnakeState new_state;
            new_state.pos = s1.pos + (s2.pos - s1.pos) * (dt) / dt_st;
            new_state.angle = s1.angle + (s2.angle - s1.angle) * (dt) / dt_st;
            new_state.dir = s1.dir;
            new_state.ent_id = s1.ent_id;
            interp_state.snakes.push_back(new_state);
        }

        return interp_state;
    };

    auto render_time = m_time_stamp - m_interpolation_delay;
    if (m_history.size() < 2 || m_history.back().server_time <= render_time)
    {
        return;
    }

    std::cout << "history: " << std::endl;
    std::size_t h_ind = 0;
    for (auto &h : m_history)
    {
        std::cout << " T: " << h.server_time << " ";
        if (h.server_time > render_time)
        {
            break;
        }
        h_ind++;
    }
    std::cout << std::endl;
    auto h_ind1 = h_ind - 1;
    if (h_ind == 0 || h_ind == m_history.size())
    {
        h_ind1 = h_ind;
    }
    auto render_state = interpolateState(m_history.at(h_ind1), m_history.at(h_ind), render_time);
    auto &ents = m_world->getEntities();
    for (auto &snake : render_state.snakes)
    {
        auto snake_p = (Snake *)(m_world->get(snake.ent_id));
        m_world->get(snake.ent_id)->setPosition(snake.pos);
        m_world->get(snake.ent_id)->setAngle(snake.angle);
        snake_p->dir = snake.dir;
    }

    while (!m_history.empty() && m_history.front().server_time + m_history_delay < m_time_stamp)
    {
        m_history.pop_front();
    }
}

struct ControlMessage
{
    std::string type;
    int dir;
};

void SnakeGame::handleEventImpl(const SDL_Event &event)
{
    nlohmann::json msg;
    msg["type"] = "control";
    msg["time"] = m_time_stamp;
    if (event.type == SDL_KEYDOWN)
    {
        auto key = event.key.keysym.sym;
        m_snake->onKeyPress(key);

        if (key == SDLK_a && m_snake->dir != 1)
        {
            msg["dir"] = 1;
            sendControlMessage(msg.dump().c_str());
        }
        else if (key == SDLK_d && m_snake->dir != -1)
        {
            msg["dir"] = -1;
            sendControlMessage(msg.dump().c_str());
        }
        else if (key == SDLK_SPACE)
        {
            msg["type"] = "poop";
            msg["dir"] = 0;
            sendControlMessage(msg.dump().c_str());
        }
    }
    else if (event.type == SDL_KEYUP)
    {
        auto key = event.key.keysym.sym;
        m_snake->onKeyRelease(key);
        if (key == SDLK_a || key == SDLK_d)
        {
            msg["dir"] = 0;
            sendControlMessage(msg.dump().c_str());
        }
    }
}

void SnakeGame::drawImpl(Renderer &window)
{
}
