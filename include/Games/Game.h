#pragma once

#include "Commands.h"
#include "GameWorld.h"
#include "DrawLayer.h"
#include "ObjectiveSystem.h"
#include "UISystem.h"
#include "Camera.h"
#include "Assets.h"
#include "Screen.h"

#include "Games/GameLevel.h"
#include "PostOffice.h"
#include "PostBox.h"
#include "Systems/TimedEventManager.h"

#include "ToolBoxUI.h"
#include "Menu/UIButtons.h"
#include "JSBindings.h"
#include "LevelLoading.h"
#include "EffectsFactory.h"

class GameWorld;

struct WordGenerator{

    WordGenerator(){}

    void setRepre(std::vector<WordRepresentation> repre);
    WordRepresentation getNext();

     std::vector<WordRepresentation> repres;
     std::size_t current_index = 0;
};

class Game : public Screen
{

public:
    Game(Renderer &window, KeyBindings &bindings, Assets& assets);
    virtual ~Game() override{}

    virtual void update(float dt) final;
    virtual void handleEvent(const SDL_Event &event) final;
    virtual void draw(Renderer &window) final;
    
    virtual void handleEventImpl(const SDL_Event& event);
    virtual void updateImpl(const float dt){}
    virtual void drawImpl(Renderer& win){}
    
    void parseInput(float dt) ;
    std::shared_ptr<GameLevelA> shooterLevelFactory(int level_id, utils::Vector2f start_pos);
    TextBubble &generateWord(bool correct, utils::Vector2f pos, utils::Vector2f size);
    TextBubble &generateWordGroup(bool correct, std::string group, utils::Vector2f pos, utils::Vector2f size);

    std::vector<WordRepresentation> generateRepre(std::vector<std::string> group,
                                                  std::string excercise_type);

    template <class... TypeList>
    void createFactories();
    

    void initializeLayersAndTextures();
    void restartGame();
    void initializeUI();
    void repositionUI();

    PostOffice messanger;

    
    Camera m_camera;
    Renderer &m_window;
    KeyBindings &m_key_binding;
    
    Player *m_playerx = nullptr;
    
    std::unique_ptr<GameWorld> m_world;
    
    Assets& m_assets;
    TextureHolder &m_textures;
    TextureHolder2 &m_atlases;
    std::shared_ptr<Font> m_font;
    
    LayersHolder m_layers;

    Sprite m_background;
    std::shared_ptr<Texture> m_background_tex;
    
    std::unique_ptr<UISystem> m_ui_system;
    std::unique_ptr<ObjectiveSystem> m_objective_system;
    std::unique_ptr<PostBox<EntityDiedEvent>> m_player_died_postbox;

    std::unordered_map<TypeId, std::unique_ptr<FactoryBase>> m_factories;
    std::unique_ptr<EffectsFactory> m_effects_factory;

    TimedEventManager m_timers;

    std::vector<std::string> m_topics;
    std::vector<WordRepresentation> m_repres;
    std::unordered_map<std::string, std::vector<WordRepresentation>> m_group_repres;
    std::unordered_map<std::string, WordGenerator> m_generators;


    std::deque<std::shared_ptr<GameLevelA>> m_levels;

    bool m_move_by_tilting = false;
    std::unordered_map<std::string, std::shared_ptr<UIButtonI>> m_buttons;

    std::string m_studied_language = "de";
};
