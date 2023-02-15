#pragma once
#include <assimp/config.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "WorldTransform.h"
#include "Texture.h"
#include "Material.h"

class BasicMesh {
public:

	BasicMesh() {};
	~BasicMesh() {};

	bool LoadMesh(const std::string& filename);

	void Render();

	void Render(unsigned int NumInstances, const glm::fmat4x4* WVPMats, const glm::fmat4x4* WorldMats);

	WorldTransform& GetWorldTransform() { return m_worldTransform; };

private:

	void Clear();

	bool InitFromScene(const aiScene* pScene, const std::string& filename);

	void ReserveSpace(unsigned int NumVertices, unsigned int NumIndices);

	void InitAllMeshes(const aiScene* pScene);

	void InitSingleMesh(const aiMesh* paiMesh);

	bool InitMaterials(const aiScene* pScene, const std::string& filename);

	void CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices);

	const Material& GetMaterial();

	void PopulateBuffers();

	enum BUFFER_TYPE {
		INDEX_BUFFER = 0,
		POS_VB = 1,
		TEXCOORD_VB = 2,
		NORMAL_VB = 3,
		WVP_MAT_VB = 4,
		WORLD_MAT_VB = 5,
		NUM_BUFFERS = 6
	};

	WorldTransform m_worldTransform;
	GLuint m_VAO = 0;
	unsigned int m_buffers[NUM_BUFFERS] = { 0 };

#define INVALID_MATERIAL 0xFFFFFFFF

	struct BasicMeshEntry {
		BasicMeshEntry() : numIndices(0), baseVertex(0), baseIndex(0), materialIndex(INVALID_MATERIAL) {};

		unsigned int numIndices;
		unsigned int baseVertex;
		unsigned int baseIndex;
		unsigned int materialIndex;
	};

	std::vector<BasicMeshEntry> m_meshes;
	std::vector<Texture*> m_textures;
	std::vector<Material> m_materials;

	//temp space before loading into gpu
	std::vector<glm::fvec3> m_positions;
	std::vector<glm::fvec3> m_normals;
	std::vector<glm::fvec2> m_texCoords;
	std::vector<unsigned int> m_indices;
};