#include "scene.h"
#include "simulation.h"
#include "renderer.h"

#include <SDL.h>
#include "mysdl_dpi.h"
#include "opengl.h"

#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"

#include <cstdio>

#define _DEBUG

extern "C"
int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_EVERYTHING))
    {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }

    // GL 4.1 for OS X support
    // Feel free to increase the GL version if your computer supports it.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#ifdef _DEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    // Don't need depth, it's done manually through the FBO.
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    // Scale window accoridng to DPI zoom
    int windowDpiScaledWidth, windowDpiScaledHeight;
    {
        int windowDpiUnscaledWidth = 1280, windowDpiUnscaledHeight = 720;

        float hdpi, vdpi, defaultDpi;
        MySDL_GetDisplayDPI(0, &hdpi, &vdpi, &defaultDpi);

        windowDpiScaledWidth = int(windowDpiUnscaledWidth * hdpi / defaultDpi);
        windowDpiScaledHeight = int(windowDpiUnscaledHeight * vdpi / defaultDpi);
    }

    Uint32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    windowFlags |= SDL_WINDOW_ALLOW_HIGHDPI;

    SDL_Window* window = SDL_CreateWindow(
        "csc305a2",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowDpiScaledWidth, windowDpiScaledHeight,
        windowFlags);
    if (!window)
    {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_GLContext glctx = SDL_GL_CreateContext(window);
    if (!glctx)
    {
        fprintf(stderr, "SDL_GL_CreateContext: %s\n", SDL_GetError());
        exit(1);
    }

    // VSync
    SDL_GL_SetSwapInterval(1);

    // Load OpenGL functions
    OpenGL_Init();

    Scene* scene = new Scene();
    scene->Init();

    Simulation* sim = new Simulation();
    sim->Init(scene);

    Renderer* renderer = new Renderer();
    renderer->Init(scene);

    ImGui_ImplSdlGL3_Init(window);

    // Initial resize to create framebuffers
    {
        int drawableWidth, drawableHeight;
        SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);

        renderer->Resize(drawableWidth, drawableHeight);
    }
    
    Uint32 then = SDL_GetTicks();

    // main loop
    for (;;)
    {
        // handle events
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&ev);

            if (ev.type == SDL_QUIT)
            {
                goto endmainloop;
            }
            else if (ev.type == SDL_WINDOWEVENT)
            {
                if (ev.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    int drawableWidth, drawableHeight;
                    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);

                    renderer->Resize(drawableWidth, drawableHeight);
                }
            }
            else if (ev.type == SDL_KEYDOWN)
            {
                // alt+enter to fullscreen
                if (ev.key.keysym.sym == SDLK_RETURN && (ev.key.keysym.mod & KMOD_ALT))
                {
                    Uint32 wflags = SDL_GetWindowFlags(window);
                    if (wflags & SDL_WINDOW_FULLSCREEN_DESKTOP)
                    {
                        SDL_SetWindowFullscreen(window, 0);
                    }
                    else
                    {
                        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    }
                }
            }

            sim->HandleEvent(ev);
        }

        ImGui_ImplSdlGL3_NewFrame(window);

        Uint32 now = SDL_GetTicks();
        Uint32 deltaTicks = now - then;

        sim->Update((float)deltaTicks / 1000.0f);

        renderer->Render();

        // Bind 0 to the draw framebuffer before swapping the window, because otherwise in Mac OS X nothing will happen. (known OSX bug)
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        SDL_GL_SwapWindow(window);

        then = now;
    }
endmainloop:

    delete renderer;
    delete sim;
    delete scene;

    ImGui_ImplSdlGL3_Shutdown();
    SDL_GL_DeleteContext(glctx);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
