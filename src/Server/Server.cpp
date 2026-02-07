#include "TestGameServer.h"

#include <Window.h>
#include <Renderer.h>
#include <DrawLayer.h>
#include <SDL2/SDL_video.h>
#include <Shader.h>

#include "ResourceFetcher.h"
#include "Utils/IOUtils.h"

#include <fstream>
#include <iostream>
#include <App.h> //! for websockets

std::uint64_t server_time = 0;

struct PerSocketData
{
    ClientEvent snake_event;
    std::size_t client_id;
    std::string name;
    uWS::WebSocket<false, true, PerSocketData> *client_socket;
};

uWS::App *p_app = nullptr;
std::unordered_set<uWS::WebSocket<false, true, PerSocketData> *> clients;

std::atomic<bool> server_running = true;
std::unordered_map<std::string, std::shared_ptr<ResourceFetcher>> m_to_fetch;

Assets loadAssets()
{
    Assets assets;

    bool fetch_locally = true;
    std::string resource_dir = {RESOURCES_DIR};

    nlohmann::json resources_data = {};
    try
    {
        resources_data = utils::loadJson((resource_dir + "Resources.json").c_str());
    }
    catch (std::exception &e)
    {
        std::cout << "Failed to load Resource Registry json!" << e.what() << std::endl;
    }

    for (auto &[resource_type, resources] : resources_data.items())
    {
        for (auto &[id, resource_data] : resources.items())
        {
            resource_data["id"] = id;
            auto fetcher = makeFetcher(resource_type, resource_data, assets, fetch_locally);
            m_to_fetch.insert({std::string(id), fetcher});
        }
    }
    for (auto &[id, fetcher] : m_to_fetch)
    {
        if (fetcher)
        {
            fetcher->doFetch();
        }
    }
    assets.textures.setBaseDirectory(resource_dir + "Textures/");
    return assets;
}

void initLayers(LayersHolder &layers, Renderer &window)
{
    auto width = window.getTargetSize().x;
    auto height = window.getTargetSize().y;

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
    auto &bg_layer = layers.addLayer("Background", 1, text_options, width, height);
    bg_layer.m_canvas.setShadersPath(shaders_directory);

    auto &unit_layer = layers.addLayer("Unit", 3, text_options, width, height);
    unit_layer.m_canvas.setShadersPath(shaders_directory);
    unit_layer.m_canvas.addShader("ElectroWall", "basicinstanced.vert", "ElectroWall.frag");
    unit_layer.m_canvas.addShader("lightningBolt", "basicinstanced.vert", "lightningBolt.frag");
    unit_layer.m_canvas.addShader("flag", "basicinstanced.vert", "flag.frag");

    double dpr = 1.;
    std::cout << "DPR IS: " << dpr << std::endl;
    auto &bloom_layer = layers.addLayer("Bloom", 5, options, width / dpr, height / dpr, dpr);
    bloom_layer.m_canvas.setShadersPath(shaders_directory);
    bloom_layer.m_canvas.addShader("ElectroWall", "basicinstanced.vert", "ElectroWall.frag");
    bloom_layer.m_canvas.addShader("gradientX", "basictex.vert", "gradientX.frag");
    bloom_layer.m_canvas.addShader("gradientY", "basictex.vert", "gradientY.frag");
    bloom_layer.addEffect(std::make_unique<BloomPhysical>(width / dpr, height / dpr, 2, 2, options));

    window.setShadersPath(shaders_directory);
    window.addShader("Shiny", "basicinstanced.vert", "shiny.frag");
    window.addShader("Arrow", "basicinstanced.vert", "texture.frag");
    window.addShader("LastPass", "basicinstanced.vert", "lastPass.frag");
}

TestGameServer *p_game = nullptr;

std::mutex msg_mutex;
std::vector<std::pair<int, nlohmann::json>> msg_queue;

//! starts a cycle that updates the game. Also starts websocket server and listeners which handle communication
void runGame()
{

    Window window(1000, 1000);
    Renderer window_canvas(window);
    Assets assets = loadAssets();
    LayersHolder layers;
    initLayers(layers, window_canvas);

    PostOffice messenger;
    TestGameServer game(messenger);
    p_game = &game;

    //! register game listener
    PostBox<nlohmann::json> game_msg_listener(messenger, [](const auto &messages)
                                              {
        for(auto& msg : messages)
        {
            nlohmann::json timed_msg = msg;
            timed_msg["time"] = server_time;
            if(p_app)
            {
                p_app->publish("game", timed_msg.dump(), uWS::TEXT);
            }
        } });

    namespace chr = std::chrono;

    chr::steady_clock clock;

    float dt = 1e6 / 30.; // 30 fps server ticks
    auto server_tick_duration = chr::microseconds((int)dt);

    auto server_start_time = clock.now();

    while (server_running)
    {
        //!
        {

            std::lock_guard guard(msg_mutex);
            while (!msg_queue.empty())
            {
                auto [client_id, data] = msg_queue.back();
                game.handleSocketEvent(client_id, data);
                msg_queue.pop_back();
            }
        }

        auto start_tic = clock.now();
        game.update(server_time);
        auto update_duration = chr::duration_cast<chr::microseconds>(clock.now() - start_tic).count();

        window_canvas.clear({0, 0, 0, 1});
        layers.clearAllLayers();
        game.draw(layers, assets, window_canvas, window_canvas.getDefaultView());
        window_canvas.drawAll();
        // layers.drawInto(window_canvas);

        SDL_GL_SwapWindow(window.getHandle()); // Swap front/back framebuffers

        server_time = chr::duration_cast<chr::microseconds>(clock.now() - server_start_time).count();
        //! broadcast server messages to clients
        messenger.distributeMessages();

        if (update_duration < server_tick_duration.count())
        {
            std::this_thread::sleep_for(std::chrono::microseconds(server_tick_duration.count() - update_duration));
        }
    }
}

void startWebsocketListeners()
{
    std::size_t next_client_id = 0;

    auto on_open = [&](auto *ws)
    {
        std::cout << "Connection Opened! " << std::endl;
        clients.insert(ws);
        ws->subscribe("game");
        ws->getUserData()->client_id = next_client_id;
        ws->getUserData()->client_socket = ws;
        nlohmann::json connection_msg;
        connection_msg["type"] = "client-id";
        connection_msg["client_id"] = next_client_id;
        ws->send(connection_msg.dump(), uWS::TEXT);
        next_client_id++;
    };

    auto on_message = [&](auto *ws, std::string_view message, uWS::OpCode opCode)
    {
        nlohmann::json message_data = nlohmann::json::parse(message);
        if (message_data.at("type") == "ping")
        {
            ws->send(message, opCode);
            return;
        }
        auto client_id = ws->getUserData()->client_id;
        std::lock_guard msg_guard(msg_mutex);
        msg_queue.push_back({client_id, message_data});
    };

    auto on_close = [&](auto *ws, int code, std::string_view message)
    {
        ws->unsubscribe("game");
        std::cout << "Connection Closed!" << std::endl;
        clients.erase(ws);
    };

    auto app =
        uWS::App().ws<PerSocketData>("/", {/* Settings */
                                           .compression = uWS::SHARED_COMPRESSOR,
                                           .maxPayloadLength = 16 * 1024,
                                           .idleTimeout = 10,
                                           /* Handlers */
                                           .open = on_open,
                                           .message = on_message,
                                           .close = on_close})
            .listen(9002, [](auto *token)
                    {
            if (token)
            {
                std::cout << "Server listening on 9002\n";
            } });

    p_app = &app;
    server_running.store(true);
    app.run();
}

int main()
{
    std::thread comm_thread([]()
                            { startWebsocketListeners(); });
    runGame();
    comm_thread.join();
    return 0;
}
