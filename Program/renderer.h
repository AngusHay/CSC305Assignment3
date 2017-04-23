#pragma once

#include "shaderset.h"

struct SDL_Window;
class Scene;

class Renderer
{
    Scene* mScene;

    // ShaderSet explanation:
    // https://nlguillemot.wordpress.com/2016/07/28/glsl-shader-live-reloading/
    ShaderSet mShaders;

    GLuint* mSceneSP;

    int mBackbufferWidth;
    int mBackbufferHeight;
    int mShadowmapWidth;
    int mShadowmapHeight;
    GLuint mBackbufferFBO;
    GLuint mShadowmapFBO;
    GLuint mBackbufferColorTO;
    GLuint mBackbufferDepthTO;
    GLuint mShadowDepthTO;

    // skybox
    GLuint mSkyboxTO;
    int mSkyboxWidth;
    int mSkyboxHeight;
    GLuint skyboxVAO;
    GLuint skyboxVBO;

    // adjustable parameters
    int mSeed = 0;
    float mCloudRed = 0.75;
    float mCloudGreen = 0.75;
    float mCloudBlue = 0.75;
	float mCloudThickness = 0.5;

    // shadowmap debugging
    GLuint* mDepthVisSP;
    GLuint mNullVAO;
    bool mShowDepthVis = true;

public:
    void Init(Scene* scene);
    void Resize(int width, int height);
    void Render();

    void* operator new(size_t sz);
};
