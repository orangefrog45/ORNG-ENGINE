#include "pch/pch.h"

#include "rendering/MeshAsset.h"
#include "util/util.h"
#include "util/Log.h"
#include "util/TimeStep.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>


using namespace ORNG;

bool MeshAsset::LoadMeshData(const std::string& raw_mesh_filepath) {
	if (m_is_loaded) {
		ORNG_CORE_TRACE("Mesh '{0}' is already loaded", raw_mesh_filepath);
		return true;
	}

	ORNG_CORE_INFO("Loading mesh: {0}", raw_mesh_filepath);
	mp_importer = std::make_unique<Assimp::Importer>();

	TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);
	bool ret = false;
	p_scene = mp_importer->ReadFile(raw_mesh_filepath.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace
		| aiProcess_ImproveCacheLocality);

	if (p_scene) {
		ret = InitFromScene(p_scene);
	}
	else {
		ORNG_CORE_ERROR("Error parsing '{0}' : '{1}'", raw_mesh_filepath.c_str(), mp_importer->GetErrorString());
		return false;
	}

	ORNG_CORE_INFO("Mesh loaded in {0}ms: {1}", time.GetTimeInterval(), raw_mesh_filepath);
	return ret;
}


bool MeshAsset::InitFromScene(const aiScene* p_scene) {
	m_submeshes.resize(p_scene->mNumMeshes);

	unsigned int num_vertices = 0;

	CountVerticesAndIndices(p_scene, num_vertices, num_indices);

	if (num_vertices > ORNG_MAX_MESH_INDICES || num_indices > ORNG_MAX_MESH_INDICES) {
		ORNG_CORE_ERROR("Mesh asset '{0}' exceeds maximum number of vertices, limit is {1}", filepath, ORNG_MAX_MESH_INDICES);
		delete p_scene;
		return false;
	}

	ReserveSpace(num_vertices, num_indices);

	InitAllMeshes(p_scene);

	return true;
}

void MeshAsset::CountVerticesAndIndices(const aiScene* pScene, unsigned int& num_vertices, unsigned int& num_indices) {
	for (unsigned int i = 0; i < m_submeshes.size(); i++) {
		m_submeshes[i].material_index = pScene->mMeshes[i]->mMaterialIndex;
		m_submeshes[i].num_indices = pScene->mMeshes[i]->mNumFaces * 3;
		m_submeshes[i].base_vertex = num_vertices;
		m_submeshes[i].base_index = num_indices;

		num_vertices += pScene->mMeshes[i]->mNumVertices;
		num_indices += m_submeshes[i].num_indices;
	}
}

void MeshAsset::ReserveSpace(unsigned int num_vertices, unsigned int num_indices) {
	// Resize here as these are memcpied into
	m_vao.vertex_data.positions.resize(num_vertices * 3);
	m_vao.vertex_data.normals.resize(num_vertices * 3);
	m_vao.vertex_data.tangents.resize(num_vertices * 3);

	m_vao.vertex_data.tex_coords.reserve(num_vertices * 2);
	m_vao.vertex_data.indices.reserve(num_indices);
}

void MeshAsset::InitAllMeshes(const aiScene* pScene) {
	unsigned current_idx = 0;
	unsigned current_vertex = 0;

	for (unsigned int i = 0; i < m_submeshes.size(); i++) {
		const aiMesh* p_ai_mesh = pScene->mMeshes[i];
		InitSingleMesh(p_ai_mesh, current_idx, current_vertex);
		current_idx += p_ai_mesh->mNumFaces * 3;
		current_vertex += p_ai_mesh->mNumVertices;
	}
}

void MeshAsset::InitSingleMesh(const aiMesh* p_ai_mesh, unsigned current_idx, unsigned current_vertex) {
	const aiVector3D zero3D{ 0.0f, 0.0f, 0.0f };

	memcpy(reinterpret_cast<std::byte*>(m_vao.vertex_data.positions.data()) + current_vertex * sizeof(glm::vec3),
		p_ai_mesh->mVertices, p_ai_mesh->mNumVertices * sizeof(glm::vec3));
	memcpy(reinterpret_cast<std::byte*>(m_vao.vertex_data.normals.data()) + current_vertex * sizeof(glm::vec3),
		p_ai_mesh->mNormals, p_ai_mesh->mNumVertices * sizeof(glm::vec3));
	memcpy(reinterpret_cast<std::byte*>(m_vao.vertex_data.tangents.data()) + current_vertex * sizeof(glm::vec3),
		p_ai_mesh->mTangents, p_ai_mesh->mNumVertices * sizeof(glm::vec3));

	for (unsigned int i = 0; i < p_ai_mesh->mNumVertices; i++) {
		const aiVector3D& pos = p_ai_mesh->mVertices[i];
		const aiVector3D& tex_coord = p_ai_mesh->HasTextureCoords(0) ? p_ai_mesh->mTextureCoords[0][i] : zero3D;

		// AABB generation
		m_aabb.extents.x = glm::max(m_aabb.extents.x, abs(pos.x));
		m_aabb.extents.y = glm::max(m_aabb.extents.y, abs(pos.y));
		m_aabb.extents.z = glm::max(m_aabb.extents.z, abs(pos.z));

		m_vao.vertex_data.tex_coords.push_back(tex_coord.x);
		m_vao.vertex_data.tex_coords.push_back(tex_coord.y);
	}

	for (unsigned int i = 0; i < p_ai_mesh->mNumFaces; i++) {
		const aiFace& face = p_ai_mesh->mFaces[i];
		m_vao.vertex_data.indices.push_back(face.mIndices[0]);
		m_vao.vertex_data.indices.push_back(face.mIndices[1]);
		m_vao.vertex_data.indices.push_back(face.mIndices[2]);
	}
}

void MeshAsset::OnLoadIntoGL() {
	m_vao.FillBuffers();
	m_is_loaded = true;

	if (mp_importer && mp_importer->GetScene())
		mp_importer->FreeScene();
}


