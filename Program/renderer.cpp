#include "renderer.h"

#include "scene.h"

#include "imgui.h"

#include "preamble.glsl"

#include "stb_image.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SDL.h>

void Renderer::Init(Scene* scene)
{
    mScene = scene;

    // feel free to increase the GLSL version if your computer supports it
    mShaders.SetVersion("410");
    mShaders.SetPreambleFile("preamble.glsl");

    mSceneSP = mShaders.AddProgramFromExts({ "scene.vert", "scene.frag" });

    float maxAnisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
}

void Renderer::Resize(int width, int height)
{
    mBackbufferWidth = width;
    mBackbufferHeight = height;

    // Init Backbuffer FBO
    {
        glDeleteTextures(1, &mBackbufferColorTO);
        glGenTextures(1, &mBackbufferColorTO);
        glBindTexture(GL_TEXTURE_2D, mBackbufferColorTO);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, mBackbufferWidth, mBackbufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDeleteTextures(1, &mBackbufferDepthTO);
        glGenTextures(1, &mBackbufferDepthTO);
        glBindTexture(GL_TEXTURE_2D, mBackbufferDepthTO);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, mBackbufferWidth, mBackbufferHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDeleteFramebuffers(1, &mBackbufferFBO);
        glGenFramebuffers(1, &mBackbufferFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, mBackbufferFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mBackbufferColorTO, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mBackbufferDepthTO, 0);
        GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "glCheckFramebufferStatus: %x\n", fboStatus);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void Renderer::Render()
{
    mShaders.UpdatePrograms();

    // Clear last frame
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mBackbufferFBO);

        glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Parameter Adjustment GUI
    if (ImGui::Begin("World Generation", 0, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::SliderInt("Seed", &mSeed, 0, 100);
    }
    if (ImGui::Button("Regenerate Scene"))
    {
        GenerateWorld((int)mSeed, mScene);
    }

    ImGui::End();

    if (ImGui::Begin("Cloud Color", 0, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::SliderFloat("Red", &mCloudRed, 0, 1);
        ImGui::SliderFloat("Green", &mCloudGreen, 0, 1);
        ImGui::SliderFloat("Blue", &mCloudBlue, 0, 1);
    }

	ImGui::End();

	if (ImGui::Begin("Cloud Thickness", 0, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::SliderFloat("", &mCloudThickness, 0, 1);
	}

    ImGui::End();

    // Clear last frame
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mBackbufferFBO);

        glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // render scene
    if (*mSceneSP)
    {
        glUseProgram(*mSceneSP);

		//Nick
		GLuint waterTexLocation = glGetUniformLocation(*mSceneSP, "waterTex");
		GLuint grassTexLocation = glGetUniformLocation(*mSceneSP, "grassTex");
		GLuint sandTexLocation = glGetUniformLocation(*mSceneSP, "sandTex");
		GLuint rockTexLocation = glGetUniformLocation(*mSceneSP, "rockTex");
		GLuint snowTexLocation = glGetUniformLocation(*mSceneSP, "snowTex");

		glUniform1i(waterTexLocation, 0);
		glUniform1i(grassTexLocation, 1);
		glUniform1i(sandTexLocation, 2);
		glUniform1i(rockTexLocation, 3);
		glUniform1i(snowTexLocation, 4);

        // GL 4.1 = no shader-specified uniform locations. :( Darn you OSX!!!
        GLint SCENE_MODELWORLD_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "ModelWorld");
        GLint SCENE_NORMAL_MODELWORLD_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "Normal_ModelWorld");
        GLint SCENE_MODELVIEWPROJECTION_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "ModelViewProjection");
        GLint SCENE_CAMERAPOS_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "CameraPos");
        GLint SCENE_LIGHTPOS_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "LightPos");
        GLint SCENE_LIGHTMATRIX_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "lightMatrix");
		GLint SCENE_CLOUD_HUE_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "CloudHue");
		GLint SCENE_CLOUD_THICKNESS_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "CloudThickness");
		GLint SCENE_HEIGHT_COLOR_TEXTURE_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "HeightColorTexture");

        const Camera& mainCamera = mScene->MainCamera;

        glm::vec3 eye = mainCamera.Eye;
        glm::vec3 up = mainCamera.Up;

        glm::mat4 worldView = glm::lookAt(eye, eye + mainCamera.Look, up);
        glm::mat4 viewProjection = glm::perspective(mainCamera.FovY, (float)mBackbufferWidth / mBackbufferHeight, 0.01f, 100.0f);
        glm::mat4 worldProjection = viewProjection * worldView;

        glProgramUniform3fv(*mSceneSP, SCENE_CAMERAPOS_UNIFORM_LOCATION, 1, value_ptr(eye));

        const Light& mainLight = mScene->MainLight;  // light source

        glm::vec3 lightPos = mainLight.Position;
        glm::vec3 lightUp = mainLight.Up;

        glm::mat4 lightWorldView = glm::lookAt(lightPos, lightPos + mainLight.Direction, lightUp);
        glm::mat4 lightViewProjection = glm::perspective(mainLight.FovY, 1.0f, 0.01f, 100.0f);
        glm::mat4 lightWorldProjection = lightViewProjection * lightWorldView;

        // does the shadowmap shader need this matrix as well?
        glm::mat4 lightOffsetMatrix = glm::mat4(
                    0.5f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.5f, 0.0f, 0.0f,
                    0.0f, 0.0f, 0.5f, 0.0f,
                    0.5f, 0.5f, 0.5f, 1.0f);
        glm::mat4 lightMatrix = lightOffsetMatrix * lightWorldProjection;

        glProgramUniform3fv(*mSceneSP, SCENE_LIGHTPOS_UNIFORM_LOCATION, 1, value_ptr(lightPos));
        glProgramUniformMatrix4fv(*mSceneSP, SCENE_LIGHTMATRIX_UNIFORM_LOCATION, 1, GL_FALSE, value_ptr(lightMatrix));

		glm::vec4 cloudHue(mCloudRed, mCloudGreen, mCloudBlue, 1);
		GLfloat cloudThickness(mCloudThickness);

		glProgramUniform4fv(*mSceneSP, SCENE_CLOUD_HUE_UNIFORM_LOCATION, 1, value_ptr(cloudHue));
		glProgramUniform1f(*mSceneSP, SCENE_CLOUD_THICKNESS_UNIFORM_LOCATION, cloudThickness);

        glBindFramebuffer(GL_FRAMEBUFFER, mBackbufferFBO);
        glViewport(0, 0, mBackbufferWidth, mBackbufferHeight);
        glEnable(GL_FRAMEBUFFER_SRGB);
        glEnable(GL_DEPTH_TEST);
        //for (uint32_t instanceID : mScene->Instances)
        for (uint32_t terrainID : mScene->Terrains)
            {
            //const Instance* instance = &mScene->Instances[instanceID];
            //const Mesh* mesh = &mScene->Meshes[instance->MeshID];
            const Terrain* terrain = &mScene->Terrains[terrainID];
            const Transform* transform = &mScene->Transforms[terrain->TransformID];


            glm::mat4 modelWorld;
            modelWorld = translate(-transform->RotationOrigin) * modelWorld;
            modelWorld = mat4_cast(transform->Rotation) * modelWorld;
            modelWorld = translate(transform->RotationOrigin) * modelWorld;
            modelWorld = scale(transform->Scale) * modelWorld;
            modelWorld = translate(transform->Translation) * modelWorld;

            glm::mat3 normal_ModelWorld;
            normal_ModelWorld = mat3_cast(transform->Rotation) * normal_ModelWorld;
            normal_ModelWorld = glm::mat3(scale(1.0f / transform->Scale)) * normal_ModelWorld;

            glm::mat4 modelViewProjection = worldProjection * modelWorld;

            glProgramUniformMatrix4fv(*mSceneSP, SCENE_MODELWORLD_UNIFORM_LOCATION, 1, GL_FALSE, value_ptr(modelWorld));
            glProgramUniformMatrix3fv(*mSceneSP, SCENE_NORMAL_MODELWORLD_UNIFORM_LOCATION, 1, GL_FALSE, value_ptr(normal_ModelWorld));
            glProgramUniformMatrix4fv(*mSceneSP, SCENE_MODELVIEWPROJECTION_UNIFORM_LOCATION, 1, GL_FALSE, value_ptr(modelViewProjection));

			//
			glActiveTexture(GL_TEXTURE0 + SCENE_HEIGHT_COLOR_MAP_TEXTURE_BINDING);
			glProgramUniform1i(*mSceneSP, SCENE_HEIGHT_COLOR_TEXTURE_UNIFORM_LOCATION, SCENE_HEIGHT_COLOR_MAP_TEXTURE_BINDING);
			glBindTexture(GL_TEXTURE_1D, mScene->heightColorTexture);
			//

            glBindVertexArray(terrain->MeshVAO);

            glDrawElementsBaseVertex(GL_TRIANGLES, (terrain->gridSize)*(terrain->gridSize)*2*3, GL_UNSIGNED_INT, 0, 0);
            //glPointSize(10);
            //glDrawElementsBaseVertex(GL_POINTS, (terrain->gridSize)*(terrain->gridSize)*2*3, GL_UNSIGNED_INT, 0, 0);

            glBindVertexArray(0);
        }



        glDisable(GL_DEPTH_TEST);
        glDisable(GL_FRAMEBUFFER_SRGB);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glUseProgram(0);
    }

    // Render ImGui
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mBackbufferFBO);
        ImGui::Render();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // copy to window
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mBackbufferFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(
            0, 0, mBackbufferWidth, mBackbufferHeight,
            0, 0, mBackbufferWidth, mBackbufferHeight,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void* Renderer::operator new(size_t sz)
{
    // zero out the memory initially, for convenience.
    void* mem = ::operator new(sz);
    memset(mem, 0, sz);
    return mem;
}
