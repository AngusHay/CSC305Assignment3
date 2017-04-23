
#pragma once

#include "opengl.h"
#include "packed_freelist.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <string>

struct DiffuseMap
{
    GLuint DiffuseMapTO;
};

struct Terrain
{
    GLuint MeshVAO;
    GLuint IndexBO;
    GLuint PositionBO;
    GLuint NormalBO;
    GLuint AltitudeBO;
	//Nick
	GLuint TexCoordBO;

    int gridSize;

    uint32_t TransformID;
};

struct ParticleSet
{
    GLuint MeshVAO;
    GLuint IndexBO;
    GLuint PositionBO;
    GLuint NormalBO;
    GLuint AltitudeBO;
    GLuint DensityBO;

    int numParticles;

    uint32_t TransformID;
};

struct Material
{
    std::string Name;

    float Ambient[3];
    float Diffuse[3];
    float Specular[3];
    float Shininess;
    uint32_t DiffuseMapID;
};

struct Mesh
{
    std::string Name;

    GLuint MeshVAO;
    GLuint PositionBO;
    GLuint TexCoordBO;
    GLuint NormalBO;
    GLuint IndexBO;

    GLuint IndexCount;
    GLuint VertexCount;

    std::vector<GLDrawElementsIndirectCommand> DrawCommands;
    std::vector<uint32_t> MaterialIDs;
};

struct Transform
{
    glm::vec3 Scale;
    glm::vec3 RotationOrigin;
    glm::quat Rotation;
    glm::vec3 Translation;
};

struct Instance
{
    uint32_t MeshID;
    uint32_t TransformID;
};

struct Camera
{
    // View
    glm::vec3 Eye;
    glm::vec3 Look;
    glm::vec3 Up;

    // Projection
    float FovY;

	bool rollercoaster = false;
	int location = 1;
	std::vector<glm::vec3> Pi_rollercoaster = { glm::vec3(1.0f, -30.0f, 0.0f), glm::vec3(1.0f, 5.0f, 0.0f), glm::vec3(12.0f, 12.0f, 0.0f), glm::vec3(12.0f, -30.0f, 0.0f) };
	glm::mat4 Mcr = glm::mat4(0.0f, -1.0f, 2.0f, -1.0f, 2.0f, 0.0f, -5.0f, 3.0f, 0.0f, 1.0f, 4.0f, -3.0f, 0.0f, 0.0f, -1.0f, 1.0f)*0.5f;
	float t_rollercoaster = 0.0f;
	float speed = 0.25f;
};

struct Light
{
    //
    glm::vec3 Position;
    glm::vec3 Direction;
    glm::vec3 Up;

    // Projection
    float FovY;
};

class Scene
{
public:
    packed_freelist<DiffuseMap> DiffuseMaps;
    packed_freelist<Material> Materials;
    packed_freelist<Mesh> Meshes;
    packed_freelist<Transform> Transforms;
    packed_freelist<Instance> Instances;

    packed_freelist<Terrain> Terrains;
    packed_freelist<ParticleSet> Particles;

	// Nick
	GLuint m_texture;

    Camera MainCamera;
    Light MainLight;

	GLuint heightColorTexture;

    void Init();
};

//Nick
void LoadTexFromFile(
	Scene* scene,
	const std::string& filename
	);

void AddMeshInstance(
    Scene* scene,
    uint32_t meshID,
    uint32_t* newInstanceID);

void GenerateTerrainMesh(
    int seed,
    Scene* scene,
    uint32_t* newTerrainID);

void GenerateWaterMesh(
    Scene* scene,
    uint32_t* newTerrainID);

void ClearTerrains(Scene* scene);

void GenerateWorld(
        int seed,
        Scene* scene);

GLuint GenerateHeightColors();