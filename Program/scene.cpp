#include "scene.h"

#include "preamble.glsl"

#include "tiny_obj_loader.h"
#include "stb_image.h"

#include <map>

#include <iostream>
#include <stack>

// for shuffle:
#include <algorithm>
#include <random>

#include <stdlib.h>   // for random

#define GRIDSIZE    120
#define TERRAINSIZE 128

void Scene::Init()
{
    // Need to specify size up front. These numbers are pretty arbitrary.
    DiffuseMaps = packed_freelist<DiffuseMap>(512);
    Materials = packed_freelist<Material>(512);
    Meshes = packed_freelist<Mesh>(512);
    Transforms = packed_freelist<Transform>(4096);
    Instances = packed_freelist<Instance>(4096);

    Terrains = packed_freelist<Terrain>(64);
    Particles = packed_freelist<ParticleSet>(4096);
}

void LoadMeshesFromFile(
    Scene* scene,
    const std::string& filename,
    std::vector<uint32_t>* loadedMeshIDs)
{
        // assume mtl is in the same folder as the obj
    std::string mtl_basepath = filename;
    size_t last_slash = mtl_basepath.find_last_of("/");
    if (last_slash == std::string::npos)
        mtl_basepath = "./";
    else
        mtl_basepath = mtl_basepath.substr(0, last_slash + 1);

    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    if (!tinyobj::LoadObj(
        shapes, materials, err, 
        filename.c_str(), mtl_basepath.c_str(),
        tinyobj::calculate_normals | tinyobj::triangulation))
    {
        fprintf(stderr, "tinyobj::LoadObj(%s) error: %s\n", filename.c_str(), err.c_str());
        return;
    }
    
    if (!err.empty())
    {
        fprintf(stderr, "tinyobj::LoadObj(%s) warning: %s\n", filename.c_str(), err.c_str());
    }






    // Add materials to the scene
    std::map<std::string, uint32_t> diffuseMapCache;
    std::vector<uint32_t> newMaterialIDs;
    for (const tinyobj::material_t& materialToAdd : materials)
    {
        Material newMaterial;

        newMaterial.Name = materialToAdd.name;

        newMaterial.Ambient[0] = materialToAdd.ambient[0];
        newMaterial.Ambient[1] = materialToAdd.ambient[1];
        newMaterial.Ambient[2] = materialToAdd.ambient[2];
        newMaterial.Diffuse[0] = materialToAdd.diffuse[0];
        newMaterial.Diffuse[1] = materialToAdd.diffuse[1];
        newMaterial.Diffuse[2] = materialToAdd.diffuse[2];
        newMaterial.Specular[0] = materialToAdd.specular[0];
        newMaterial.Specular[1] = materialToAdd.specular[1];
        newMaterial.Specular[2] = materialToAdd.specular[2];
        newMaterial.Shininess = materialToAdd.shininess;

        newMaterial.DiffuseMapID = -1;

        if (!materialToAdd.diffuse_texname.empty())
        {
            auto cachedTexture = diffuseMapCache.find(materialToAdd.diffuse_texname);

            if (cachedTexture != end(diffuseMapCache))
            {
                newMaterial.DiffuseMapID = cachedTexture->second;
            }
            else
            {
                std::string diffuse_texname_full = mtl_basepath + materialToAdd.diffuse_texname;
                int x, y, comp;
                stbi_set_flip_vertically_on_load(1);
                stbi_uc* pixels = stbi_load(diffuse_texname_full.c_str(), &x, &y, &comp, 4);
                stbi_set_flip_vertically_on_load(0);

                if (!pixels)
                {
                    fprintf(stderr, "stbi_load(%s): %s\n", diffuse_texname_full.c_str(), stbi_failure_reason());
                }
                else
                {
                    float maxAnisotropy;
                    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

                    GLuint newDiffuseMapTO;
                    glGenTextures(1, &newDiffuseMapTO);
                    glBindTexture(GL_TEXTURE_2D, newDiffuseMapTO);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, 0);

                    DiffuseMap newDiffuseMap;
                    newDiffuseMap.DiffuseMapTO = newDiffuseMapTO;
                    
                    uint32_t newDiffuseMapID = scene->DiffuseMaps.insert(newDiffuseMap);

                    diffuseMapCache.emplace(materialToAdd.diffuse_texname, newDiffuseMapID);

                    newMaterial.DiffuseMapID = newDiffuseMapID;

                    stbi_image_free(pixels);
                }
            }
        }

        uint32_t newMaterialID = scene->Materials.insert(newMaterial);

        newMaterialIDs.push_back(newMaterialID);
    }

    // Add meshes (and prototypes) to the scene
    for (const tinyobj::shape_t& shapeToAdd : shapes)
    {
        const tinyobj::mesh_t& meshToAdd = shapeToAdd.mesh;

        Mesh newMesh;

        newMesh.Name = shapeToAdd.name;

        newMesh.IndexCount = (GLuint)meshToAdd.indices.size();
        newMesh.VertexCount = (GLuint)meshToAdd.positions.size() / 3;

        if (meshToAdd.positions.empty())
        {
            // should never happen
            newMesh.PositionBO = 0;
        }
        else
        {
            GLuint newPositionBO;
            glGenBuffers(1, &newPositionBO);
            glBindBuffer(GL_ARRAY_BUFFER, newPositionBO);
            glBufferData(GL_ARRAY_BUFFER, meshToAdd.positions.size() * sizeof(meshToAdd.positions[0]), meshToAdd.positions.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            newMesh.PositionBO = newPositionBO;
        }

        if (meshToAdd.texcoords.empty())
        {
            newMesh.TexCoordBO = 0;
        }
        else
        {
            GLuint newTexCoordBO;
            glGenBuffers(1, &newTexCoordBO);
            glBindBuffer(GL_ARRAY_BUFFER, newTexCoordBO);
            glBufferData(GL_ARRAY_BUFFER, meshToAdd.texcoords.size() * sizeof(meshToAdd.texcoords[0]), meshToAdd.texcoords.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            newMesh.TexCoordBO = newTexCoordBO;
        }

        if (meshToAdd.normals.empty())
        {
            newMesh.NormalBO = 0;
        }
        else
        {
            GLuint newNormalBO;
            glGenBuffers(1, &newNormalBO);
            glBindBuffer(GL_ARRAY_BUFFER, newNormalBO);
            glBufferData(GL_ARRAY_BUFFER, meshToAdd.normals.size() * sizeof(meshToAdd.normals[0]), meshToAdd.normals.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            newMesh.NormalBO = newNormalBO;
        }

        if (meshToAdd.indices.empty())
        {
            // should never happen
            newMesh.IndexBO = 0;
        }
        else
        {
            GLuint newIndexBO;
            glGenBuffers(1, &newIndexBO);
            // Why not bind to GL_ELEMENT_ARRAY_BUFFER?
            // Because binding to GL_ELEMENT_ARRAY_BUFFER attaches the EBO to the currently bound VAO, which might stomp somebody else's state.
            glBindBuffer(GL_ARRAY_BUFFER, newIndexBO);
            glBufferData(GL_ARRAY_BUFFER, meshToAdd.indices.size() * sizeof(meshToAdd.indices[0]), meshToAdd.indices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            newMesh.IndexBO = newIndexBO;
        }

        // Hook up VAO
        {
            GLuint newMeshVAO;
            glGenVertexArrays(1, &newMeshVAO);

            glBindVertexArray(newMeshVAO);

            if (newMesh.PositionBO)
            {
                glBindBuffer(GL_ARRAY_BUFFER, newMesh.PositionBO);
                glVertexAttribPointer(SCENE_POSITION_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glEnableVertexAttribArray(SCENE_POSITION_ATTRIB_LOCATION);
            }

            if (newMesh.TexCoordBO)
            {
                glBindBuffer(GL_ARRAY_BUFFER, newMesh.TexCoordBO);
                glVertexAttribPointer(SCENE_TEXCOORD_ATTRIB_LOCATION, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glEnableVertexAttribArray(SCENE_TEXCOORD_ATTRIB_LOCATION);
            }

            if (newMesh.NormalBO)
            {
                glBindBuffer(GL_ARRAY_BUFFER, newMesh.NormalBO);
                glVertexAttribPointer(SCENE_NORMAL_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glEnableVertexAttribArray(SCENE_NORMAL_ATTRIB_LOCATION);
            }

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newMesh.IndexBO);

            glBindVertexArray(0);

            newMesh.MeshVAO = newMeshVAO;
        }

        // split mesh into draw calls with different materials
        int numFaces = (int)meshToAdd.indices.size() / 3;
        int currMaterialFirstFaceIndex = 0;
        for (int faceIdx = 0; faceIdx < numFaces; faceIdx++)
        {
            bool isLastFace = faceIdx + 1 == numFaces;
            bool isNextFaceDifferent = isLastFace || meshToAdd.material_ids[faceIdx + 1] != meshToAdd.material_ids[faceIdx];
            if (isNextFaceDifferent)
            {
                GLDrawElementsIndirectCommand currDrawCommand;
                currDrawCommand.count = ((faceIdx + 1) - currMaterialFirstFaceIndex) * 3;
                currDrawCommand.primCount = 1;
                currDrawCommand.firstIndex = currMaterialFirstFaceIndex * 3;
                currDrawCommand.baseVertex = 0;
                currDrawCommand.baseInstance = 0;

                uint32_t currMaterialID = newMaterialIDs[meshToAdd.material_ids[faceIdx]];

                newMesh.DrawCommands.push_back(currDrawCommand);
                newMesh.MaterialIDs.push_back(currMaterialID);

                currMaterialFirstFaceIndex = faceIdx + 1;
            }
        }

        uint32_t newMeshID = scene->Meshes.insert(newMesh);

        if (loadedMeshIDs)
        {
            loadedMeshIDs->push_back(newMeshID);
        }
    }
}

//Nick
void LoadTexFromFile(Scene* scene, const std::string& filename)
{
	int width, height, numComponents;
	stbi_uc* imageData = stbi_load((filename.c_str()), &width, &height, &numComponents, 4);

	if (imageData == NULL)
		std::cerr << "Texture loading failed: " << filename << std::endl;
	else
	{
		float maxAnisotropy;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

		GLuint newDiffuseMapTO = scene->m_texture;
		glGenTextures(1, &newDiffuseMapTO);
		glBindTexture(GL_TEXTURE_2D, newDiffuseMapTO);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		stbi_image_free(imageData);
	}
}

// perlin noise-related functions
// taken from https://connex.csc.uvic.ca/access/content/group/feae68d4-a86f-4c6a-af57-9a21e665f7d0/Reading%20Material/Noise/Understanding%20Perlin%20Noise.pdf

int p[] = {
    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    151,160,137,91,90,15,   // after this point, the permutation repeats
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

int ps[512];   // shuffled table

double fade(double t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double lerp(double a, double b, double x) {
    return a + x * (b - a);
}

// 2d select gradient
double grad(int hash, double x, double y){
    switch(hash & 0x3)
    {
    case 0x0: return x + y;
    case 0x1: return -x + y;
    case 0x2: return x - y;
    case 0x3: return -x - y;
    default: return 0; // never happens
    }
}

// 3d select gradient
double grad(int hash, double x, double y, double z){
    switch(hash & 0xF)
    {
    case 0x0: return x + y;
    case 0x1: return -x + y;
    case 0x2: return x - y;
    case 0x3: return -x - y;
    case 0x4: return x + z;
    case 0x5: return -x + z;
    case 0x6: return x - z;
    case 0x7: return -x - z;
    case 0x8: return y + z;
    case 0x9: return -y + z;
    case 0xA: return y - z;
    case 0xB: return -y - z;
    case 0xC: return y + x;
    case 0xD: return -y + z;
    case 0xE: return y - x;
    case 0xF: return -y - z;
    default: return 0; // never happens
    }
}

// 2d perlin noise
double perlin(double x, double y) {
    int xi = (int)x & 255;
    int yi = (int)y & 255;

    double xf = x - (int)x;
    double yf = y - (int)y;

    double u = fade(xf);
    double v = fade(yf);

    /*int aa, ab, ba, bb;
    aa = p[p[xi] + yi];
    ab = p[p[xi] + yi+1];
    ba = p[p[xi+1] + yi];
    bb = p[p[xi+1] + yi+1];*/

    int aa, ab, ba, bb;
    aa = ps[ps[xi] + yi];
    ab = ps[ps[xi] + yi+1];
    ba = ps[ps[xi+1] + yi];
    bb = ps[ps[xi+1] + yi+1];

    double x1, x2;
    x1 = lerp( grad (aa, xf , yf),
               grad (ba, xf-1, yf),
               u);
    x2 = lerp( grad (ab, xf , yf-1),
               grad (bb, xf-1, yf-1),
               u);
    return(lerp(x1, x2, v)+1)/2;
}

// 3d perlin noise
double perlin(double x, double y, double z) {
    int xi = (int)x & 255;
    int yi = (int)y & 255;
    int zi = (int)z & 255;

    double xf = x - (int)x;
    double yf = y - (int)y;
    double zf = z - (int)z;

    double u = fade(xf);
    double v = fade(yf);
    double w = fade(zf);

    /*int aa, ab, ba, bb;
    aa = p[p[xi] + yi];
    ab = p[p[xi] + yi+1];
    ba = p[p[xi+1] + yi];
    bb = p[p[xi+1] + yi+1];*/

    int aaa, aba, aab, abb, baa, bba, bab, bbb;
    aaa = ps[ps[ps[ xi ]+ yi ]+ zi ];
    aba = ps[ps[ps[ xi ]+yi+1]+ zi ];
    aab = ps[ps[ps[ xi ]+ yi ]+zi+1];
    abb = ps[ps[ps[ xi ]+yi+1]+zi+1];
    baa = ps[ps[ps[xi+1]+ yi ]+ zi ];
    bba = ps[ps[ps[xi+1]+yi+1]+ zi ];
    bab = ps[ps[ps[xi+1]+ yi ]+zi+1];
    bbb = ps[ps[ps[xi+1]+yi+1]+zi+1];

    double x1, x2, y1, y2;
    x1 = lerp( grad (aaa, xf , yf , zf),
               grad (baa, xf-1, yf , zf),
               u);
    x2 = lerp( grad (aba, xf , yf-1, zf),
               grad (bba, xf-1, yf-1, zf),
               u);
    y1 = lerp(x1, x2, v);
    x1 = lerp( grad (aab, xf , yf , zf-1),
               grad (bab, xf-1, yf , zf-1),
               u);
    x2 = lerp( grad (abb, xf , yf-1, zf-1),
               grad (bbb, xf-1, yf-1, zf-1),
               u);
    y2 = lerp (x1, x2, v);

    return (lerp (y1, y2, w)+1)/2;
}

// perlin function produces values generally between 0.25 and 0.75, so this function creates values between 0 and 1
// note: there may be some clustering at 0 and 1
double perlin_clamped(double x, double y) {
    double value = perlin(x, y);
    value -= 0.25;
    value *= 2.0;
    return fmax(0, fmin(1, value));
}

// this function is used to ensure that shuffle uses rand(), so it can be seeded
unsigned int random_for_p(int n) {
    return rand() % n;
}

void AddMeshInstance(
    Scene* scene,
    uint32_t meshID,
    uint32_t* newInstanceID)
{
    Transform newTransform;
    newTransform.Scale = glm::vec3(1.0f);

    uint32_t newTransformID = scene->Transforms.insert(newTransform);

    Instance newInstance;
    newInstance.MeshID = meshID;
    newInstance.TransformID = newTransformID;

    uint32_t tmpNewInstanceID = scene->Instances.insert(newInstance);
    if (newInstanceID)
    {
        *newInstanceID = tmpNewInstanceID;
    }
}

void GenerateTerrainMesh(int seed, Scene* scene, uint32_t* newTerrainID) {
    srand(seed);

    Terrain terrain;

    terrain.gridSize = GRIDSIZE;

    Transform newTransform;
    newTransform.Scale = glm::vec3(1.0f);

    uint32_t newTransformID = scene->Transforms.insert(newTransform);

    terrain.TransformID = newTransformID;

    int perlin_iterations = 6;

    float height = 8.0;

    // if the seed is 0, use the default permutation
    if(seed != 0) {
        // copy and re-order the Perlin noise permutation table randomly
        for(int i = 0; i < 256; i++) {
            ps[i] = p[i];
        }
        //std::shuffle(&ps[0], &ps[256], random_for_p);
		std::random_shuffle(std::begin(ps), std::end(ps));
		for(int i = 256; i < 511; i++) {
            ps[i] = ps[i - 256];
        }
    } else {
        for(int i = 0; i < 256; i++) {
            ps[i] = p[i];
            ps[i+256] = p[i];
        }
    }

    // from "Procedural Fractal Terrains", F. Kenton Musgrave
    float H = 0.25;
    float lacunarity = 2.0;
    int octaves = 8;
    //float exponent_array[octaves];
	float* exponent_array = new float[octaves];

    float frequency = 1.0;
    for(int i = 0; i < octaves; i++) {
        exponent_array[i] = pow(frequency, -H);
        frequency *= lacunarity;
    }

	// end code from "Procedural Fractal Terrains"

    // points for a 2x2 terrain
    GLfloat terrainMeshVerticies[(GRIDSIZE+1)*(GRIDSIZE+1)][3];
    for(int i = 0; i < (GRIDSIZE+1); i++) {
        for(int j = 0; j < (GRIDSIZE+1); j++) {
            terrainMeshVerticies[i*(GRIDSIZE+1) + j][0] = 0.125 * (i * 1.0f - GRIDSIZE/2) * TERRAINSIZE / GRIDSIZE;
            terrainMeshVerticies[i*(GRIDSIZE+1) + j][2] = 0.125 * (j * 1.0f - GRIDSIZE/2) * TERRAINSIZE / GRIDSIZE;

            terrainMeshVerticies[i*(GRIDSIZE+1) + j][1] = 0;

            // hybrid multifractal, from Musgrave
            float x = (float)i / GRIDSIZE;
            float y = (float)j / GRIDSIZE;
            float value = 0;

            value += perlin(x, y) * 2 * exponent_array[0];
            float weight = value;

            x *= lacunarity;
            y *= lacunarity;

            for(int k = 1; k < octaves; k++) {
                if(weight > 2.0) weight = 2.0;

                float signal = perlin(x, y) * 2 * exponent_array[k];
                value += signal * weight;

                weight *= signal;

                x *= lacunarity;
                y *= lacunarity;
            }

			// end code from "Procedural Fractal Terrains"

            terrainMeshVerticies[i*(GRIDSIZE+1) + j][1] = value + 0.75;

            // basic fBm algorithm
            // H = 1, lacunarity = 2, ocaves = 6
            /*
            float x = (float)i / GRIDSIZE;
            float y = (float)j / GRIDSIZE;
            float value = 0;
            for(int k = 0; k < octaves; k++) {
                value += perlin(x, y) * exponent_array[k];
                x *= lacunarity;
                y *= lacunarity;
            }

            terrainMeshVerticies[i*(GRIDSIZE+1) + j][1] = value * 8.0 - 4.0;
            */

            // original implementation
            /*
            float perlin_multiplier = 1.0;

            for(int k = 0; k < perlin_iterations; k++) {
                terrainMeshVerticies[i*(GRIDSIZE+1) + j][1] += (height / perlin_multiplier) * perlin((float)i * perlin_multiplier / GRIDSIZE, (float)j * perlin_multiplier / GRIDSIZE);
                perlin_multiplier *= 2.0;
            }

            terrainMeshVerticies[i*(GRIDSIZE+1) + j][1] -= height / 2;
            */

        }
    }

    GLuint newPositionBO;
    glGenBuffers(1, &newPositionBO);
    glBindBuffer(GL_ARRAY_BUFFER, newPositionBO);
    glBufferData(GL_ARRAY_BUFFER, (GRIDSIZE+1) * (GRIDSIZE+1) * 3 * sizeof(GLfloat), terrainMeshVerticies, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    terrain.PositionBO = newPositionBO;

	//Currently commented out to avoid a stack overflow
	/*
	//Nick
	GLuint terrainMeshTexCoord[(GRIDSIZE)*(GRIDSIZE)][2][3][2];
	for (int i = 0; i < GRIDSIZE; i++) {
		for (int j = 0; j < GRIDSIZE; j++) {
			//top left
			terrainMeshTexCoord[i*(GRIDSIZE)+j][0][0][0] = 0;
			terrainMeshTexCoord[i*(GRIDSIZE)+j][0][0][1] = 1;
			//bottom left
			terrainMeshTexCoord[i*(GRIDSIZE)+j][0][1][0] = 0;
			terrainMeshTexCoord[i*(GRIDSIZE)+j][0][1][1] = 0;
			//top right
			terrainMeshTexCoord[i*(GRIDSIZE)+j][0][2][0] = 1;
			terrainMeshTexCoord[i*(GRIDSIZE)+j][0][2][1] = 1;
			//bottom right
			terrainMeshTexCoord[i*(GRIDSIZE)+j][1][0][0] = 1;
			terrainMeshTexCoord[i*(GRIDSIZE)+j][1][0][1] = 0;
			//bottom left
			terrainMeshTexCoord[i*(GRIDSIZE)+j][1][1][0] = 0;
			terrainMeshTexCoord[i*(GRIDSIZE)+j][1][1][1] = 0;
			//top right
			terrainMeshTexCoord[i*(GRIDSIZE)+j][1][2][0] = 1;
			terrainMeshTexCoord[i*(GRIDSIZE)+j][1][2][1] = 1;
		}
	}

	GLuint newTexCoordBO;
	glGenBuffers(1, &newTexCoordBO);
	glBindBuffer(GL_ARRAY_BUFFER, newTexCoordBO);
	glBufferData(GL_ARRAY_BUFFER, GRIDSIZE * GRIDSIZE * 2 * 3 * 2 * sizeof(terrainMeshTexCoord[0][0][0][0]), terrainMeshTexCoord, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	terrain.TexCoordBO = newTexCoordBO;
	*/

    GLuint TerrainIndexBO[GRIDSIZE*GRIDSIZE][2][3];    // 100 squares, 2 triangles/square, 3 points/triangle
    for(int i = 0; i < GRIDSIZE; i++) {
        for(int j = 0; j < GRIDSIZE; j++) {
            // triangle 1
            TerrainIndexBO[i*(GRIDSIZE) + j][0][0] = i*(GRIDSIZE+1) + j;
            TerrainIndexBO[i*(GRIDSIZE) + j][0][1] = (i+1) * (GRIDSIZE+1) + j;
            TerrainIndexBO[i*(GRIDSIZE) + j][0][2] = i*(GRIDSIZE+1) + j + 1;
            // triangle 2
            TerrainIndexBO[i*(GRIDSIZE) + j][1][0] = (i+1) * (GRIDSIZE+1) + j + 1;
            TerrainIndexBO[i*(GRIDSIZE) + j][1][1] = (i+1) * (GRIDSIZE+1) + j;
            TerrainIndexBO[i*(GRIDSIZE) + j][1][2] = i*(GRIDSIZE+1) + j + 1;
        }
    }

    glm::vec3 normals[(GRIDSIZE+1)*(GRIDSIZE+1)];

    for(int i = 0; i < GRIDSIZE*GRIDSIZE; i++) {
        GLuint t1_i[3] = {TerrainIndexBO[i][0][0], TerrainIndexBO[i][0][1], TerrainIndexBO[i][0][2]};   // indicies of vertices of first triangle
        glm::vec3 t1_a(terrainMeshVerticies[t1_i[0]][0], terrainMeshVerticies[t1_i[0]][1], terrainMeshVerticies[t1_i[0]][2]);
        glm::vec3 t1_b(terrainMeshVerticies[t1_i[1]][0], terrainMeshVerticies[t1_i[1]][1], terrainMeshVerticies[t1_i[1]][2]);
        glm::vec3 t1_c(terrainMeshVerticies[t1_i[2]][0], terrainMeshVerticies[t1_i[2]][1], terrainMeshVerticies[t1_i[2]][2]);

        normals[TerrainIndexBO[i][0][0]] += normalize(cross(t1_b - t1_a, t1_b - t1_c));
        normals[TerrainIndexBO[i][0][1]] += normalize(cross(t1_b - t1_a, t1_b - t1_c));
        normals[TerrainIndexBO[i][0][2]] += normalize(cross(t1_b - t1_a, t1_b - t1_c));

        GLuint t2_i[3] = {TerrainIndexBO[i][1][0], TerrainIndexBO[i][1][1], TerrainIndexBO[i][1][2]};   // indicies of vertices of second triangle
        glm::vec3 t2_a(terrainMeshVerticies[t1_i[0]][0], terrainMeshVerticies[t1_i[0]][1], terrainMeshVerticies[t1_i[0]][2]);
        glm::vec3 t2_b(terrainMeshVerticies[t1_i[1]][0], terrainMeshVerticies[t1_i[1]][1], terrainMeshVerticies[t1_i[1]][2]);
        glm::vec3 t2_c(terrainMeshVerticies[t1_i[2]][0], terrainMeshVerticies[t1_i[2]][1], terrainMeshVerticies[t1_i[2]][2]);

        normals[TerrainIndexBO[i][1][0]] += normalize(cross(t1_b - t1_a, t1_b - t1_c));
        normals[TerrainIndexBO[i][1][1]] += normalize(cross(t1_b - t1_a, t1_b - t1_c));
        normals[TerrainIndexBO[i][1][2]] += normalize(cross(t1_b - t1_a, t1_b - t1_c));


        //normals[2*i] = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));
        //normals[2*i + 1] = glm::normalize(glm::vec3(0.0f, 1.0f, 1.0f));
    }

    for(int i = 0; i < (GRIDSIZE+1)*(GRIDSIZE+1); i++) {
        normals[i] = normalize(normals[i]);
    }

    GLuint newIndexBO;
    glGenBuffers(1, &newIndexBO);
    // Why not bind to GL_ELEMENT_ARRAY_BUFFER?
    // Because binding to GL_ELEMENT_ARRAY_BUFFER attaches the EBO to the currently bound VAO, which might stomp somebody else's state.
    glBindBuffer(GL_ARRAY_BUFFER, newIndexBO);
    glBufferData(GL_ARRAY_BUFFER, GRIDSIZE * GRIDSIZE * 2 * 3 * sizeof(TerrainIndexBO[0][0][0]), TerrainIndexBO, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    terrain.IndexBO = newIndexBO;

    GLuint newNormalBO;

    glGenBuffers(1, &newNormalBO);
    glBindBuffer(GL_ARRAY_BUFFER, newNormalBO);
    glBufferData(GL_ARRAY_BUFFER, (GRIDSIZE+1)*(GRIDSIZE+1) * sizeof(normals[0]), normals, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    terrain.NormalBO = newNormalBO;


    GLuint newMeshVAO;
    glGenVertexArrays(1, &newMeshVAO);

    glBindVertexArray(newMeshVAO);

    if (terrain.PositionBO)
    {
        glBindBuffer(GL_ARRAY_BUFFER, terrain.PositionBO);
        glVertexAttribPointer(SCENE_POSITION_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glEnableVertexAttribArray(SCENE_POSITION_ATTRIB_LOCATION);
    }

    glBindBuffer(GL_ARRAY_BUFFER, terrain.NormalBO);
    glVertexAttribPointer(SCENE_NORMAL_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glEnableVertexAttribArray(SCENE_NORMAL_ATTRIB_LOCATION);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrain.IndexBO);

    glBindVertexArray(0);

    terrain.MeshVAO = newMeshVAO;


    uint32_t tmpNewTerrainID = scene->Terrains.insert(terrain);
    if (newTerrainID)
    {
        *newTerrainID = tmpNewTerrainID;
    }
}

void GenerateWaterMesh(Scene* scene, uint32_t* newTerrainID) {
    Terrain water;

    water.gridSize = GRIDSIZE;

    Transform newTransform;
    newTransform.Scale = glm::vec3(1.0f);

    uint32_t newTransformID = scene->Transforms.insert(newTransform);

    water.TransformID = newTransformID;

    // points for a 100x100 water mesh
    GLfloat waterMeshVerticies[(GRIDSIZE+1)*(GRIDSIZE+1)][3];

	for (int i = 0; i < GRIDSIZE+1; i++) {
		for (int j = 0; j < GRIDSIZE+1; j++) {
			waterMeshVerticies[i * (GRIDSIZE + 1) + j][0] = 0.125 * (i * 1.0f - GRIDSIZE / 2) * TERRAINSIZE / GRIDSIZE;
			waterMeshVerticies[i * (GRIDSIZE + 1) + j][1] = 2.745;
			waterMeshVerticies[i * (GRIDSIZE + 1) + j][2] = 0.125 * (j * 1.0f - GRIDSIZE / 2) * TERRAINSIZE / GRIDSIZE;
		}
	}

    GLuint newPositionBO;
    glGenBuffers(1, &newPositionBO);
    glBindBuffer(GL_ARRAY_BUFFER, newPositionBO);
    glBufferData(GL_ARRAY_BUFFER, (GRIDSIZE+1)*(GRIDSIZE+1) * 3 * sizeof(GLfloat), waterMeshVerticies, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    water.PositionBO = newPositionBO; 
	
	GLuint WaterIndexBO[GRIDSIZE*GRIDSIZE][2][3];    // 100 squares, 2 triangles/square, 3 points/triangle
	for (int i = 0; i < GRIDSIZE; i++) {
		for (int j = 0; j < GRIDSIZE; j++) {
			// triangle 1
			WaterIndexBO[i*(GRIDSIZE)+j][0][0] = i*(GRIDSIZE + 1) + j;
			WaterIndexBO[i*(GRIDSIZE)+j][0][1] = (i + 1) * (GRIDSIZE + 1) + j;
			WaterIndexBO[i*(GRIDSIZE)+j][0][2] = i*(GRIDSIZE + 1) + j + 1;
			// triangle 2
			WaterIndexBO[i*(GRIDSIZE)+j][1][0] = (i + 1) * (GRIDSIZE + 1) + j + 1;
			WaterIndexBO[i*(GRIDSIZE)+j][1][1] = (i + 1) * (GRIDSIZE + 1) + j;
			WaterIndexBO[i*(GRIDSIZE)+j][1][2] = i*(GRIDSIZE + 1) + j + 1;
		}
	}

    glm::vec3 normals[(GRIDSIZE + 1)*(GRIDSIZE + 1)];

    for(int i = 0; i < (GRIDSIZE + 1)*(GRIDSIZE + 1); i++) {
        normals[i] = glm::vec3(0.0f, 1.0f, 0.0f);
        normals[i] = normalize(normals[i]); // just in case
    }

    GLuint newIndexBO;
    glGenBuffers(1, &newIndexBO);
    // Why not bind to GL_ELEMENT_ARRAY_BUFFER?
    // Because binding to GL_ELEMENT_ARRAY_BUFFER attaches the EBO to the currently bound VAO, which might stomp somebody else's state.
    glBindBuffer(GL_ARRAY_BUFFER, newIndexBO);
    glBufferData(GL_ARRAY_BUFFER, (GRIDSIZE + 1)*(GRIDSIZE + 1) * 2 * 3 * sizeof(WaterIndexBO[0][0][0]), WaterIndexBO, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    water.IndexBO = newIndexBO;

    GLuint newNormalBO;

    glGenBuffers(1, &newNormalBO);
    glBindBuffer(GL_ARRAY_BUFFER, newNormalBO);
    glBufferData(GL_ARRAY_BUFFER, (GRIDSIZE + 1)*(GRIDSIZE + 1) * sizeof(normals[0]), normals, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    water.NormalBO = newNormalBO;


    GLuint newMeshVAO;
    glGenVertexArrays(1, &newMeshVAO);

    glBindVertexArray(newMeshVAO);

    if (water.PositionBO)
    {
        glBindBuffer(GL_ARRAY_BUFFER, water.PositionBO);
        glVertexAttribPointer(SCENE_POSITION_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glEnableVertexAttribArray(SCENE_POSITION_ATTRIB_LOCATION);
    }

    glBindBuffer(GL_ARRAY_BUFFER, water.NormalBO);
    glVertexAttribPointer(SCENE_NORMAL_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glEnableVertexAttribArray(SCENE_NORMAL_ATTRIB_LOCATION);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, water.IndexBO);

    glBindVertexArray(0);

    water.MeshVAO = newMeshVAO;


    uint32_t tmpNewTerrainID = scene->Terrains.insert(water);
    if (newTerrainID)
    {
        *newTerrainID = tmpNewTerrainID;
    }
}

void ClearTerrains(Scene* scene) {
    for (uint32_t terrainID : scene->Terrains) {
        if(scene->Terrains.contains(terrainID)) {
            scene->Terrains.erase(terrainID);
        }
    }
    for (uint32_t particlesID : scene->Particles) {
        if(scene->Particles.contains(particlesID)) {
            scene->Particles.erase(particlesID);
        }
    }
}

/*void GenerateClouds(int seed, Scene* scene, uint32_t* newCloudsID) {
    srand(seed + 1);   // do a bitwise flip to get a diffe

    ParticleSet clouds;

    int maxCloudRes = 150;

    Transform newTransform;
    newTransform.Scale = glm::vec3(1.0f);

    uint32_t newTransformID = scene->Transforms.insert(newTransform);

    clouds.TransformID = newTransformID;

    int perlin_iterations = 6;

    float height = 8.0;

    // copy and re-order the Perlin noise permutation table randomly
    for(int i = 0; i < 256; i++) {
        ps[i] = p[i];
    }
   // std::shuffle(&ps[0], &ps[256], random_for_p);
    std::random_shuffle(&ps[0], &ps[256]);
    for(int i = 256; i < 511; i++) {
        ps[i] = ps[i - 256];
    }

    // from "Procedural Fractal Terrains", F. Kenton Musgrave
    float H = 1.0;
    float lacunarity = 2.0;
    const int octaves = 4;
    double exponent_array[octaves];

    float frequency = 1.0;
    for(int i = 0; i < octaves; i++) {
        exponent_array[i] = pow(frequency, -H);
        frequency *= lacunarity;
    }
	// end code from "Procedural Fractal Terrains"


    std::stack <glm::vec4> cloudPoints;

    int verticiesToDraw = 0;
    for(int i = 0; i < (maxCloudRes+1); i++) {
        for(int j = 0; j < (maxCloudRes+1); j++) {
            for(int k = 0; k < (maxCloudRes+1); k++) {
                int arrayIndex = i*(maxCloudRes+1)*(maxCloudRes+1) + j*(maxCloudRes+1) + k;

                GLfloat pointX = 0.125 * (i * 1.0f - maxCloudRes/2 + (rand() % 10 - 5) * 0.1) * TERRAINSIZE / maxCloudRes;
                GLfloat pointY = 8 + k * 0.2 + 0.125 * ((rand() % 10 - 5) * 0.1) * TERRAINSIZE / maxCloudRes;
                GLfloat pointZ = 0.125 * (j * 1.0f - maxCloudRes/2 + (rand() % 10 - 5) * 0.1) * TERRAINSIZE / maxCloudRes;

                // basic fBm algorithm
                // H = 1, lacunarity = 2, ocaves = 6
                float x = (float)i / (maxCloudRes/3);
                float y = (float)j / (maxCloudRes/3);
                float z = (float)k / (maxCloudRes/3);
                float value = 0;
                for(int l = 0; l < octaves; l++) {
                    value += (perlin(x, y, z) - 0.5) * exponent_array[l];
                    x *= lacunarity;
                    y *= lacunarity;
                }



                value = (value + 0.5) * (1 - fabs(k - ((maxCloudRes)/2.0)) / (maxCloudRes/2.0)) - 0.5;

                if(value > 0) {
                    cloudPoints.push(glm::vec4(pointX, pointY, pointZ, value));
                    verticiesToDraw++;
                }
            }
        }
    }

    clouds.numParticles = verticiesToDraw;

    //GLfloat cloudPointVerticies[verticiesToDraw][3];
    //GLfloat cloudPointDensities[verticiesToDraw];
	// changed because Visual Studio doesn't support dynamic array declaration
	GLfloat** cloudPointVerticies = new GLfloat*[verticiesToDraw];
	GLfloat* cloudPointDensities = new GLfloat[verticiesToDraw];
	for (int i = 0; i < verticiesToDraw; i++) {
		cloudPointVerticies[i] = new GLfloat[3];
	}
	for(int i = 0; i < verticiesToDraw; i++) {
        glm::vec4 currPoint = cloudPoints.top();
        cloudPoints.pop();
        cloudPointVerticies[i][0] = currPoint.x;
        cloudPointVerticies[i][1] = currPoint.y;
        cloudPointVerticies[i][2] = currPoint.z;
        cloudPointDensities[i] = fmin(currPoint.w * 4, 1);
    }

    GLuint newPositionBO;
    glGenBuffers(1, &newPositionBO);
    glBindBuffer(GL_ARRAY_BUFFER, newPositionBO);
    //glBufferData(GL_ARRAY_BUFFER, (GRIDSIZE+1) * (GRIDSIZE+1) * 3 * sizeof(GLfloat), cloudPointVerticies, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, verticiesToDraw * 3 * sizeof(GLfloat), cloudPointVerticies, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    clouds.PositionBO = newPositionBO;

    //GLuint TerrainIndexBO[verticiesToDraw];    // 100 squares, 2 triangles/square, 3 points/triangle
	GLuint* TerrainIndexBO = new GLuint[verticiesToDraw];
	for(int i = 0; i < verticiesToDraw; i++) {
        TerrainIndexBO[i] = i;
    }

    glm::vec3 *normals = new glm::vec3[verticiesToDraw];
    for(int i = 0; i < verticiesToDraw; i++) {
        normals[i] += normalize(glm::vec3(0, 1, 0));
    }

    GLuint newIndexBO;
    glGenBuffers(1, &newIndexBO);
    // Why not bind to GL_ELEMENT_ARRAY_BUFFER?
    // Because binding to GL_ELEMENT_ARRAY_BUFFER attaches the EBO to the currently bound VAO, which might stomp somebody else's state.
    glBindBuffer(GL_ARRAY_BUFFER, newIndexBO);
    //glBufferData(GL_ARRAY_BUFFER, (GRIDSIZE+1) * (GRIDSIZE+1) * sizeof(TerrainIndexBO[0]), TerrainIndexBO, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, verticiesToDraw * 3 * sizeof(TerrainIndexBO[0]), TerrainIndexBO, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    clouds.IndexBO = newIndexBO;

    GLuint newNormalBO;

    glGenBuffers(1, &newNormalBO);
    glBindBuffer(GL_ARRAY_BUFFER, newNormalBO);
    //glBufferData(GL_ARRAY_BUFFER, (GRIDSIZE+1)*(GRIDSIZE+1) * sizeof(normals[0]), normals, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, verticiesToDraw * sizeof(normals[0]), normals, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    clouds.NormalBO = newNormalBO;

    GLuint newDensityBO;

    glGenBuffers(1, &newDensityBO);
    glBindBuffer(GL_ARRAY_BUFFER, newDensityBO);
    glBufferData(GL_ARRAY_BUFFER, verticiesToDraw * sizeof(cloudPointDensities[0]), cloudPointDensities, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    clouds.DensityBO = newDensityBO;


    GLuint newMeshVAO;
    glGenVertexArrays(1, &newMeshVAO);

    glBindVertexArray(newMeshVAO);

    if (clouds.PositionBO)
    {
        glBindBuffer(GL_ARRAY_BUFFER, clouds.PositionBO);
        glVertexAttribPointer(SCENE_POSITION_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glEnableVertexAttribArray(SCENE_POSITION_ATTRIB_LOCATION);
    }

    if(clouds.NormalBO) {
        glBindBuffer(GL_ARRAY_BUFFER, clouds.NormalBO);
        glVertexAttribPointer(SCENE_NORMAL_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glEnableVertexAttribArray(SCENE_NORMAL_ATTRIB_LOCATION);
    }

    if(clouds.DensityBO) {
        glBindBuffer(GL_ARRAY_BUFFER, clouds.DensityBO);
        glVertexAttribPointer(SCENE_DENSITY_ATTRIB_LOCATION, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glEnableVertexAttribArray(SCENE_DENSITY_ATTRIB_LOCATION);
    }


    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, clouds.IndexBO);

    glBindVertexArray(0);

    clouds.MeshVAO = newMeshVAO;


    uint32_t tmpNewTerrainID = scene->Particles.insert(clouds);
    if (newCloudsID)
    {
        *newCloudsID = tmpNewTerrainID;
    }
}*/

void GenerateWorld(int seed, Scene* scene) {

    ClearTerrains(scene);

    uint32_t newTerrainID;

    GenerateTerrainMesh(seed, scene, &newTerrainID);

    uint32_t waterID;

    GenerateWaterMesh(scene, &waterID);

	scene->heightColorTexture = GenerateHeightColors();
}

GLuint GenerateHeightColors() {
	GLuint heightColorTex;
	glGenTextures(1, &heightColorTex);

	glBindTexture(GL_TEXTURE_1D, heightColorTex);

	glm::vec4 snow = glm::vec4(1.0, 1.0, 1.0, 1.0);
	glm::vec4 rock = glm::vec4(0.45, 0.45, 0.45, 1.0);
	glm::vec4 highForest = glm::vec4(0.0, 0.6, 0.25, 1.0);
	glm::vec4 lowForest = glm::vec4(0.0, 0.5, 0.2, 1.0);
	glm::vec4 beach = glm::vec4(0.7, 0.7, 0.3, 1.0);
	glm::vec4 water = glm::vec4(0, 0.2, 0.8, 1.0);


	glm::vec4 heightColors[1000];
	for (int i = 0; i < 275; i++) {
		heightColors[i] = water;
	}
	for (int i = 0; i < 45; i++) {
		heightColors[i + 275] = (i / 45.0f) * lowForest + (1.0f - i / 45.0f) * beach;
	}
	for (int i = 0; i < 15; i++) {
		heightColors[i + 320] = lowForest;
	}
	for (int i = 0; i < 45; i++) {
		heightColors[i + 335] = ((i) / 45.0f) * highForest + (1.0f - (i) / 45.0f) * lowForest;
	}
	for (int i = 0; i < 15; i++) {
		heightColors[i + 380] = highForest;
	}
	for (int i = 0; i < 55; i++) {
		heightColors[i + 395] = ((i) / 55.0f) * rock + (1.0f - (i) / 55.0f) * highForest;
	}
	for (int i = 450; i < 500; i++) {
		heightColors[i] = rock;
	}
	for (int i = 500; i < 550; i++) {
		heightColors[i] = ((i - 500) / 50.0f) * snow + (1.0f - (i - 500) / 50.0f) * rock;
	}
	for (int i = 550; i < 1000; i++) {
		heightColors[i] = snow;
	}

	for (int i = 0; i < 1000; i+= 100) {
		std::cout << heightColors[i].x << ", " << heightColors[i].y << ", " << heightColors[i].z << std::endl;
	}

	glTexImage1D(GL_TEXTURE_1D, 0, GL_SRGB8_ALPHA8, 1000, 0, GL_RGBA, GL_FLOAT, heightColors);
	glGenerateMipmap(GL_TEXTURE_1D);

	//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_1D, 0);

	//std::cout << heightColors[500].x << ", " << heightColors[500].y << ", " << heightColors[500].z << std::endl;
	//std::cout << heightColorTex << std::endl;

	return heightColorTex;
}