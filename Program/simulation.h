#pragma once

#include <cstddef>

struct SDL_Window;
union SDL_Event;
class Scene;

class Simulation
{
    Scene* mScene;

    int mDeltaMouseX;
    int mDeltaMouseY;

public:
    void Init(Scene* scene);
    void HandleEvent(const SDL_Event& ev);
    void Update(float deltaTime);

    void* operator new(size_t sz);

    //void GenerateWorld(int seed);
};

//Nick
void BindTex(Scene *scene, const unsigned int unit);