#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdlib.h>
#include <glm/gtx/matrix_major_storage.hpp>
#include <iostream>
#include <glew.h>
#include <glfw/glfw3.h>
#include <future>
#include <format>
#include "MeshData.h"
#include "util/util.h"

static constexpr unsigned int POSITION_LOCATION = 0;
static constexpr unsigned int TEX_COORD_LOCATION = 1;
static constexpr unsigned int NORMAL_LOCATION = 2;
static constexpr unsigned int WORLD_MAT_LOCATION_1 = 3;
static constexpr unsigned int WORLD_MAT_LOCATION_2 = 4;
static constexpr unsigned int WORLD_MAT_LOCATION_3 = 5;
static constexpr unsigned int WORLD_MAT_LOCATION_4 = 6;


void MeshData::UnloadMesh() {
	m_meshes.clear();
	m_textures.clear();
	m_materials.clear();
	m_positions.clear();
	m_normals.clear();
	m_texCoords.clear();
	m_indices.clear();
	importer.FreeScene();
	PrintUtils::PrintSuccess("Mesh unloaded: " + m_filename);
	is_loaded = false;
}

void MeshData::LoadIntoGL() {
	GLCall(glGenVertexArrays(1, &m_VAO));
	GLCall(glGenBuffers(ARRAY_SIZE_IN_ELEMENTS(m_buffers), m_buffers));
	GLCall(glBindVertexArray(m_VAO));
	InitMaterials(p_scene, m_filename);
	PopulateBuffers();
	importer.FreeScene();
	GLCall(glBindVertexArray(0));
	is_loaded = true;
}


bool MeshData::LoadMeshData() {

	PrintUtils::PrintDebug("Loading mesh: " + m_filename);
	double start_time = glfwGetTime();

	bool ret = false;

	p_scene = importer.ReadFile(m_filename.c_str(), ASSIMP_LOAD_FLAGS);

	if (p_scene) {
		ret = InitFromScene(p_scene, m_filename);
	}
	else {
		printf("Error parsing '%s': '%s'\n", m_filename.c_str(), importer.GetErrorString());
	}


	PrintUtils::PrintSuccess(std::format("Mesh loaded in {}ms: {}", PrintUtils::RoundDouble((glfwGetTime() - start_time) * 1000), m_filename));
	return ret;
}

bool MeshData::InitFromScene(const aiScene* pScene, const std::string& filename) {
	m_meshes.resize(pScene->mNumMeshes);
	m_textures.resize(pScene->mNumMaterials);
	m_materials.resize(pScene->mNumMaterials);

	unsigned int numVertices = 0;
	unsigned int numIndices = 0;

	CountVerticesAndIndices(pScene, numVertices, numIndices);

	ReserveSpace(numVertices, numIndices);

	InitAllMeshes(pScene);

	return true;
}

void MeshData::CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices) {

	for (unsigned int i = 0; i < m_meshes.size(); i++) {

		m_meshes[i].materialIndex = pScene->mMeshes[i]->mMaterialIndex;
		m_meshes[i].numIndices = pScene->mMeshes[i]->mNumFaces * 3;
		m_meshes[i].baseVertex = NumVertices;
		m_meshes[i].baseIndex = NumIndices;

		NumVertices += pScene->mMeshes[i]->mNumVertices;
		NumIndices += m_meshes[i].numIndices;
	}
}

void MeshData::ReserveSpace(unsigned int NumVertices, unsigned int NumIndices) {
	m_positions.reserve(NumVertices);
	m_normals.reserve(NumVertices);
	m_texCoords.reserve(NumVertices);
	m_indices.reserve(NumIndices);
}

void MeshData::InitAllMeshes(const aiScene* pScene) {
	for (unsigned int i = 0; i < m_meshes.size(); i++) {
		const aiMesh* paiMesh = pScene->mMeshes[i];
		InitSingleMesh(paiMesh);
	}
}

void MeshData::InitSingleMesh(const aiMesh* paiMesh) {

	const aiVector3D zero3D(0.0f, 0.0f, 0.0f);

	//populate vertex attribute vectors
	for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
		const aiVector3D& pPos = paiMesh->mVertices[i];
		const aiVector3D& pNormal = paiMesh->mNormals[i];
		const aiVector3D& pTexCoord = paiMesh->HasTextureCoords(0) ? paiMesh->mTextureCoords[0][i] : zero3D;

		m_positions.push_back(glm::fvec3(pPos.x, pPos.y, pPos.z));
		m_normals.push_back(glm::fvec3(pNormal.x, pNormal.y, pNormal.z));
		m_texCoords.push_back(glm::fvec2(pTexCoord.x, pTexCoord.y));
	}

	for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
		const aiFace& face = paiMesh->mFaces[i];
		ASSERT(face.mNumIndices == 3);
		m_indices.push_back(face.mIndices[0]);
		m_indices.push_back(face.mIndices[1]);
		m_indices.push_back(face.mIndices[2]);
	}

}

bool MeshData::InitMaterials(const aiScene* pScene, const std::string& filename) {
	std::string::size_type slashIndex = filename.find_last_of("/");
	std::string dir;

	if (slashIndex == std::string::npos) {
		dir = ".";
	}
	else if (slashIndex == 0) {
		dir = "/";
	}
	else {
		dir = filename.substr(0, slashIndex);
	}


	for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
		const aiMaterial* pMaterial = pScene->mMaterials[i];
		LoadTextures(dir, pMaterial, i);
		LoadColors(pMaterial, i);

	}
	return true;
}

void MeshData::LoadColors(const aiMaterial* pMaterial, unsigned int index) {
	aiColor3D AmbientColor(0.0f, 0.0f, 0.0f);

	if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, AmbientColor) == aiReturn_SUCCESS && AmbientColor != aiColor3D(0.0f, 0.0f, 0.0f)) {
		m_materials[index].ambient_color.r = AmbientColor.r;
		m_materials[index].ambient_color.g = AmbientColor.g;
		m_materials[index].ambient_color.b = AmbientColor.b;
	}

	aiColor3D diffuse_color(0.0f, 0.0f, 0.0f);

	if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, diffuse_color) == aiReturn_SUCCESS && diffuse_color != aiColor3D(0.0f, 0.0f, 0.0f)) {
		m_materials[index].diffuse_color.r = diffuse_color.r;
		m_materials[index].diffuse_color.g = diffuse_color.g;
		m_materials[index].diffuse_color.b = diffuse_color.b;
	}

	aiColor3D specular_color(0.0f, 0.0f, 0.0f);

	if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, specular_color) == aiReturn_SUCCESS && specular_color != aiColor3D(0.0f, 0.0f, 0.0f)) {
		m_materials[index].specular_color.r = specular_color.r;
		m_materials[index].specular_color.g = specular_color.g;
		m_materials[index].specular_color.b = specular_color.b;
	}
};


void MeshData::LoadTextures(const std::string& t_dir, const aiMaterial* pMaterial, unsigned int index) {
	LoadDiffuseTexture(t_dir, pMaterial, index);
	LoadSpecularTexture(t_dir, pMaterial, index);
}

void MeshData::LoadDiffuseTexture(const std::string& t_dir, const aiMaterial* pMaterial, unsigned int index) {
	m_materials[index].diffuse_texture = nullptr;

	if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
		aiString path;

		if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS) {
			std::string p(path.data);

			if (p.substr(0, 2) == ".\\") {
				p = p.substr(2, p.size() - 2);
			}

			std::string fullPath = t_dir + "/" + p;

			m_materials[index].diffuse_texture = std::make_shared<Texture>(GL_TEXTURE_2D, fullPath.c_str());

			if (!m_materials[index].diffuse_texture->Load()) {
				PrintUtils::PrintError("Error loading diffuse texture at: " + fullPath);
				m_materials[index].diffuse_texture = std::make_shared<Texture>(GL_TEXTURE_2D, "./res/textures/missing_texture.jpeg");
				m_materials[index].diffuse_texture->Load();
			}
			else {
				PrintUtils::PrintSuccess("Loaded diffuse texture: " + fullPath);
			}
		}

	}
	else {
		PrintUtils::PrintWarning("No diffuse texture found at: " + t_dir);
		m_materials[index].diffuse_texture = std::make_shared<Texture>(GL_TEXTURE_2D, "./res/textures/missing_texture.jpeg");
		m_materials[index].diffuse_texture->Load();
	}
}

void MeshData::LoadSpecularTexture(const std::string& t_dir, const aiMaterial* pMaterial, unsigned int index) {
	m_materials[index].specular_texture = nullptr;

	if (pMaterial->GetTextureCount(aiTextureType_SHININESS) > 0) {
		aiString path;

		if (pMaterial->GetTexture(aiTextureType_SHININESS, 0, &path, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS) {
			std::string p(path.data);

			if (p.substr(0, 2) == ".\\") {
				p = p.substr(2, p.size() - 2);
			}

			std::string fullPath = t_dir + "/" + p;

			m_materials[index].specular_texture = std::make_shared<Texture>(GL_TEXTURE_2D, fullPath.c_str());

			if (!m_materials[index].specular_texture->Load()) {
				PrintUtils::PrintError("Error loading specular texture: " + fullPath);
			}
			else {
				PrintUtils::PrintSuccess("Loaded specular texture: " + fullPath);
			}
		}

	}
	else {
		PrintUtils::PrintWarning("No shininess texture found at: " + t_dir);
		m_materials[index].specular_texture = nullptr;
	}
}


void MeshData::PopulateBuffers() {
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_buffers[POS_VB]));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(m_positions[0]) * m_positions.size(), &m_positions[0], GL_STATIC_DRAW));
	GLCall(glEnableVertexAttribArray(POSITION_LOCATION));
	GLCall(glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_buffers[TEXCOORD_VB]));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(m_texCoords[0]) * m_texCoords.size(), &m_texCoords[0], GL_STATIC_DRAW));
	GLCall(glEnableVertexAttribArray(TEX_COORD_LOCATION));
	GLCall(glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_buffers[NORMAL_VB]));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(m_normals[0]) * m_normals.size(), &m_normals[0], GL_STATIC_DRAW));
	GLCall(glEnableVertexAttribArray(NORMAL_LOCATION));
	GLCall(glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0));

	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers[INDEX_BUFFER]));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_indices[0]) * m_indices.size(), &m_indices[0], GL_STATIC_DRAW));

}

void MeshData::UpdateTransformBuffers(const std::vector<WorldTransform const*>& transforms) {

	std::vector<glm::fmat4> gl_transforms;

	for (WorldTransform const* transform : transforms) {
		gl_transforms.push_back(glm::rowMajor4(transform->GetMatrix()));
	}

	GLCall(glBindVertexArray(m_VAO));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_buffers[WORLD_MAT_VB]));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(gl_transforms[0]) * gl_transforms.size(), &gl_transforms[0], GL_DYNAMIC_DRAW));

	GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_1));
	GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0));
	GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_1, 1));

	GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_2));
	GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4))));
	GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_2, 1));

	GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_3));
	GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * 2)));
	GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_3, 1));

	GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_4));
	GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * 3)));
	GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_4, 1));

	GLCall(glBindVertexArray(0));

}
