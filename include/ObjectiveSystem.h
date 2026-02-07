#pragma once

#include <queue>
#include <string>
#include <functional>
#include <unordered_set>
#include <fstream>

#include <Renderer.h>
#include <Font.h>

#include "Utils/ObjectPool.h"

#include "PostBox.h"
#include "GameEvents.h"
#include "TimedEvent.h"

#include "Menu/UIDocument.h"

class UIDrawerVisitor
{
public:
    UIDrawerVisitor(UIDocument &ui)
        : m_ui(ui) {}

    // void visit(ReachSpotTask& task)
    // {
    //     auto task_bar = m_ui.getElementById("TaskBlock");
    //     if(task_bar)
    //     {

    //     }
    // }

private:
    UIDocument &m_ui;
};

class Quest;
class Task
{

public:
    Task(PostOffice &messenger, Quest *parent)
        : m_post_office(&messenger), m_parent(parent)
    {
    }

    virtual ~Task() = default;
    Task(const Task &other) = default;
    Task(Task &&other) = default;
    Task &operator=(const Task &other) = default;
    Task &operator=(Task &&other) = default;

    virtual void update(float dt)
    {
        for(auto& t_comp : m_timer_component)
        {
            t_comp.update(dt);

        }
    };
    virtual void draw(Renderer &window, const TextureHolder &textures) = 0;

    void activate();
    void complete();
    void fail();

    bool isFinished() const;

    // virtual void accept(UIDrawerVisitor &visitor) = 0;

public:
    std::function<void()> m_on_completion_callback = []() {};
    std::function<void()> m_on_failure_callback = []() {};
    std::function<void()> m_on_activation = []() {};

    int m_id;

    std::vector<TimedEvent> m_timer_component;

protected:
    PostOffice *m_post_office = nullptr;
    Quest *m_parent = nullptr;
    Font *m_font = nullptr;

    std::vector<std::unique_ptr<PostBoxI>> m_listeners;
    
    bool m_is_finished = false;
    bool m_active = false;
};

class CompositeTask : public Task
{

public:
    CompositeTask(PostOffice &messenger, std::vector<std::unique_ptr<Task>> &tasks, Quest *parent);

    void onTaskCompletion(Task *completed_task);
    void addTask(std::unique_ptr<Task> sub_task);
    virtual void draw(Renderer &window, const TextureHolder &textures) override;

private:
    int m_tasks_count_to_complete = 0;
    std::vector<std::unique_ptr<Task>> sub_tasks;
};

enum class QuestState
{
    NotStarted,
    Locked,
    Active,
    Completed,
    Failed
};

class Quest
{
public:
    float m_text_y_pos; //! TODO: will create some draw compotent some time

    Quest(PostOffice &messenger);

    void draw(Renderer &window, const TextureHolder &textures);
    void addTask(std::shared_ptr<Task> task, Task *precondition = nullptr);
    void onTaskCompletion(Task *completed_task);
    void update(float dt);
    void completeQuest();
    void start();

public:
    std::function<void()> m_on_failure = []() {};
    std::function<void()> m_on_completion = []() {};
    std::function<void(Quest &)> m_on_start = [](Quest &) {};

private:
    struct TaskGraphNode
    {
        std::shared_ptr<Task> task;
        Task *precondition;
        std::vector<Task *> children;
    };

    PostOffice *p_post_office;

    int root_task_id = 0;
    std::vector<TaskGraphNode> sub_tasks;
    std::unordered_set<int> m_active_tasks_ids;

    int m_id = 0;

    QuestState m_state = QuestState::NotStarted;
    Task *primary_task;
};

class SurviveTask : public Task
{
public:
SurviveTask(GameObject &entity, PostOffice &messenger, Quest *parent);
    virtual ~SurviveTask() = default;

    virtual void draw(Renderer &window, const TextureHolder &textures) override;

private:
    std::unique_ptr<PostBox<EntityDiedEvent>> m_death_postbox;
};



class DestroyEntityTask : public Task
{

public:
    DestroyEntityTask(GameObject &target, Font &font, PostOffice &messenger, Quest *parent);
    virtual ~DestroyEntityTask() = default;

    virtual void draw(Renderer &window, const TextureHolder &textures) override;

private:
    GameObject &m_target;
    std::unique_ptr<PostBox<EntityDiedEvent>> m_postbox;
};

class DestroyNOfTypeTask : public Task
{

public:
    DestroyNOfTypeTask(ObjectType type, std::string name, int destroyed_target_count, Font &font, PostOffice &messenger, Quest *parent);
    virtual ~DestroyNOfTypeTask() = default;

    void entityDestroyed(ObjectType type, int id);

    virtual void draw(Renderer &window, const TextureHolder &textures) override;

private:
    ObjectType m_type;
    int m_destroyed_count = 0;
    int m_destroyed_target;

    std::unique_ptr<PostBox<EntityDiedEvent>> m_on_entity_destroyed;

    std::string m_entity_name;
};

class PostOffice;

class ObjectiveSystem
{

public:
    ObjectiveSystem(PostOffice &messanger);

    void add(std::shared_ptr<Quest> quest);
    void remove(int id);
    bool contains(std::shared_ptr<Quest> quest) const
    {
        return m_quests.contains(quest);
    }
    void draw(Renderer &window, const TextureHolder &textures);
    void update(float dt);
    bool allFinished() const;
    void entityDestroyed(ObjectType type, int id);

    void registerQuest(std::filesystem::path json);

private:
    // std::deque<std::shared_ptr<Objective>> m_objectives;

    std::unordered_set<std::shared_ptr<Quest>> m_quests;

    std::unordered_set<int> active_quests_ids;

    bool m_all_quests_finished = false;

    PostOffice *p_messanger;

    std::unique_ptr<PostBox<EntityDiedEvent>> post_box;
    std::unique_ptr<PostBox<QuestCompletedEvent>> m_objectives_postbox;
};