#include "simulation.h"

#include "scene.h"

#include "imgui.h"

#define FLYTHROUGH_CAMERA_IMPLEMENTATION
#include "flythrough_camera.h"

#include <glm/gtc/type_ptr.hpp>

#include <SDL.h>

void Simulation::Init(Scene* scene)
{
    mScene = scene;

	//Nick
	LoadTexFromFile(scene, "assets/water.tga");
	BindTex(scene, 0);
	LoadTexFromFile(scene, "assets/grass.tga");
	BindTex(scene, 1);
	LoadTexFromFile(scene, "assets/sand.tga");
	BindTex(scene, 2);
	LoadTexFromFile(scene, "assets/rock.tga");
	BindTex(scene, 3);
	LoadTexFromFile(scene, "assets/snow.tga");
	BindTex(scene, 4);

    Camera mainCamera;
    mainCamera.Eye = glm::vec3(0.0f, 5.0f, 8.0f);
    glm::vec3 target = glm::vec3(0.0f);
    mainCamera.Look = normalize(target - mainCamera.Eye);
    mainCamera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
    mainCamera.FovY = glm::radians(70.0f);
    mScene->MainCamera = mainCamera;

    // light source
    Light mainLight;
    mainLight.Position = glm::vec3(5, 5, 5);
    glm::vec3 lightTarget = glm::vec3(0.0f);
    mainLight.Direction = normalize(lightTarget - mainLight.Position);
    mainLight.Up = glm::vec3(0.0f, 1.0f, 0.0f);
    mainLight.FovY = glm::radians(70.0f);
    mScene->MainLight = mainLight;

    GenerateWorld(0, mScene);
	//mScene->heightColorTexture = GenerateHeightColors();
}

//Nick
void BindTex(Scene *scene, const unsigned int unit) {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, scene->m_texture);
}

void Simulation::HandleEvent(const SDL_Event& ev)
{
    if (ev.type == SDL_MOUSEMOTION)
    {
        mDeltaMouseX += ev.motion.xrel;
        mDeltaMouseY += ev.motion.yrel;
    }
}

void Simulation::Update(float deltaTime)
{
    const Uint8* keyboard = SDL_GetKeyboardState(NULL);
    
    int mx, my;
    Uint32 mouse = SDL_GetMouseState(&mx, &my);

	if((mouse & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0 && mScene->MainCamera.rollercoaster == false)
	{
		flythrough_camera_update(
			value_ptr(mScene->MainCamera.Eye),
			value_ptr(mScene->MainCamera.Look),
			value_ptr(mScene->MainCamera.Up),
			NULL,
			deltaTime,
			5.0f, // eye_speed
			0.1f, // degrees_per_cursor_move
			80.0f, // max_pitch_rotation_degrees
			mDeltaMouseX, mDeltaMouseY,
			keyboard[SDL_SCANCODE_W], keyboard[SDL_SCANCODE_A], keyboard[SDL_SCANCODE_S], keyboard[SDL_SCANCODE_D],
			keyboard[SDL_SCANCODE_SPACE], keyboard[SDL_SCANCODE_LCTRL],
			0);
	}
	//use a bezier curve to animate the camera path - Jordan Patterson
	if (mScene->MainCamera.rollercoaster == true) {
		int j = mScene->MainCamera.location;
		std::vector <glm::vec3> Pi = mScene->MainCamera.Pi_rollercoaster;
		glm::mat4 Mcr = mScene->MainCamera.Mcr;
		mScene->MainCamera.t_rollercoaster += mScene->MainCamera.speed * deltaTime;
		float t = mScene->MainCamera.t_rollercoaster;
		float t2 = t*t;
		float t3 = t*t*t;

		glm::vec4 T = glm::vec4(1, t, t2, t3);
		glm::vec4 Cix = glm::vec4(Pi[j - 1].x, Pi[j].x, Pi[j + 1].x, Pi[j + 2].x);
		glm::vec4 Ciy = glm::vec4(Pi[j - 1].y, Pi[j].y, Pi[j + 1].y, Pi[j + 2].y);
		glm::vec4 Ciz = glm::vec4(Pi[j - 1].z, Pi[j].z, Pi[j + 1].z, Pi[j + 2].z);
		glm::mat3x4 Ci = glm::mat3x4(Cix, Ciy, Ciz);
		//glm::vec3 CiT = Mcr * T * Ci;
		glm::vec3 CiT = T * Mcr * Ci;
		//glm::vec3 CiT = glm::vec3((Cix * Mcr * T), (Ciy * Mcr * T), (Cix * Mcr * T));

		mScene->MainCamera.Eye = CiT;
		mScene->MainCamera.Look = glm::normalize(0.0f - mScene->MainCamera.Eye);

		mScene->MainCamera.location++;
		if (mScene->MainCamera.location == Pi.size() - 2) {
			mScene->MainCamera.location = 1;
		}
		if (mScene->MainCamera.t_rollercoaster > 1.0) {
			mScene->MainCamera.t_rollercoaster = 0.0;
		}
	}

	mDeltaMouseX = 0;
	mDeltaMouseY = 0;

	//mScene->MainCamera.Pi_rollercoaster
	if (ImGui::Begin("Catmull-Rom Spline"))
	{
		ImGui::Checkbox("Rollercoaster", &mScene->MainCamera.rollercoaster);
		ImGui::SliderFloat("Speed", &mScene->MainCamera.speed, 0, 1);
		ImGui::PushItemWidth(87.5);
		ImGui::InputFloat("P1x", &mScene->MainCamera.Pi_rollercoaster[0].x);
		ImGui::SameLine();
		ImGui::InputFloat("P1y", &mScene->MainCamera.Pi_rollercoaster[0].y);
		ImGui::SameLine();
		ImGui::InputFloat("P1z", &mScene->MainCamera.Pi_rollercoaster[0].z);
		ImGui::InputFloat("P2x", &mScene->MainCamera.Pi_rollercoaster[1].x);
		ImGui::SameLine();
		ImGui::InputFloat("P2y", &mScene->MainCamera.Pi_rollercoaster[1].y);
		ImGui::SameLine();
		ImGui::InputFloat("P2z", &mScene->MainCamera.Pi_rollercoaster[1].z);
		ImGui::InputFloat("P3x", &mScene->MainCamera.Pi_rollercoaster[2].x);
		ImGui::SameLine();
		ImGui::InputFloat("P3y", &mScene->MainCamera.Pi_rollercoaster[2].y);
		ImGui::SameLine();
		ImGui::InputFloat("P3z", &mScene->MainCamera.Pi_rollercoaster[2].z);
		ImGui::InputFloat("P4x", &mScene->MainCamera.Pi_rollercoaster[3].x);
		ImGui::SameLine();
		ImGui::InputFloat("P4y", &mScene->MainCamera.Pi_rollercoaster[3].y);
		ImGui::SameLine();
		ImGui::InputFloat("P4z", &mScene->MainCamera.Pi_rollercoaster[3].z);
		ImGui::PopItemWidth();
	}
	ImGui::End();
}

void* Simulation::operator new(size_t sz)
{
    // zero out the memory initially, for convenience.
    void* mem = ::operator new(sz);
    memset(mem, 0, sz);
    return mem;
}

/*
void Simulation::GenerateWorld(int seed) {
    ClearTerrains(mScene);

    uint32_t newTerrainID;

    GenerateTerrainMesh(0, mScene, &newTerrainID);

    uint32_t waterID;

    GenerateWaterMesh(mScene, &waterID);
}*/
