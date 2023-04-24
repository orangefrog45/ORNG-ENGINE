#include "pch/pch.h"

#include <glm/gtx/matrix_major_storage.hpp>
#include "rendering/MeshAsset.h"
#include "util/util.h"
#include "util/Log.h"
#include "WorldTransform.h"
#include "TimeStep.h"

static constexpr unsigned int POSITION_LOCATION = 0;
static constexpr unsigned int TEX_COORD_LOCATION = 1;
static constexpr unsigned int NORMAL_LOCATION = 2;
static constexpr unsigned int WORLD_MAT_LOCATION_1 = 3;
static constexpr unsigned int WORLD_MAT_LOCATION_2 = 4;
static constexpr unsigned int WORLD_MAT_LOCATION_3 = 5;
static constexpr unsigned int WORLD_MAT_LOCATION_4 = 6;
static constexpr unsigned int TANGENT_LOCATION = 7;


void MeshAsset::UnloadMesh() {
	m_meshes.clear();

	for (auto& material : m_materials) {
		delete material.diffuse_texture;
		delete material.normal_map_texture;
		delete material.specular_texture;
	}

	m_materials.clear();

	m_positions.clear();
	m_normals.clear();
	m_texCoords.clear();
	m_indices.clear();
	m_tangents.clear();
	importer.FreeScene();

	GLCall(glDeleteBuffers(ARRAY_SIZE_IN_ELEMENTS(m_buffers), m_buffers));
	GLCall(glDeleteBuffers(1, &m_aabb_pos_vb));

	GLenum vertex_arrays[] = { m_VAO, m_aabb_vao };
	GLCall(glDeleteVertexArrays(2, vertex_arrays));

	OAR_CORE_INFO("Mesh unloaded: {0}", m_filename);
	is_loaded = false;
}

bool MeshAsset::LoadMeshData() {

	if (is_loaded) {
		OAR_CORE_TRACE("Mesh '{0}' is already loaded", m_filename);
		return true;
	}

	OAR_CORE_INFO("Loading mesh: {0}", m_filename);


	if (m_filename.empty()) {
		OAR_CORE_CRITICAL("Mesh load failed, no filename given");
		BREAKPOINT;
	}

	TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);

	bool ret = false;
	p_scene = importer.ReadFile(m_filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace
		| aiProcess_ImproveCacheLocality);

	if (p_scene) {
		ret = InitFromScene(p_scene, m_filename);
	}
	else {
		OAR_CORE_ERROR("Error parsing '{0}' : '{1}'", m_filename.c_str(), importer.GetErrorString());
		return false;
	}

	OAR_CORE_INFO("Mesh loaded in {0}ms: {1}", time.GetTimeInterval(), m_filename);
	return ret;
}

bool MeshAsset::InitFromScene(const aiScene* pScene, const std::string& filename) {
	m_meshes.resize(pScene->mNumMeshes);
	m_materials.resize(pScene->mNumMaterials);

	unsigned int num_vertices = 0;

	CountVerticesAndIndices(pScene, num_vertices, num_indices);

	ReserveSpace(num_vertices, num_indices);

	InitAllMeshes(pScene);

	return true;
}

void MeshAsset::CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices) {

	for (unsigned int i = 0; i < m_meshes.size(); i++) {

		m_meshes[i].materialIndex = pScene->mMeshes[i]->mMaterialIndex;
		m_meshes[i].numIndices = pScene->mMeshes[i]->mNumFaces * 3;
		m_meshes[i].baseVertex = NumVertices;
		m_meshes[i].baseIndex = NumIndices;

		NumVertices += pScene->mMeshes[i]->mNumVertices;
		NumIndices += m_meshes[i].numIndices;
	}
}

void MeshAsset::ReserveSpace(unsigned int NumVertices, unsigned int NumIndices) {
	m_positions.reserve(NumVertices);
	m_normals.reserve(NumVertices);
	m_tangents.reserve(NumVertices);
	m_texCoords.reserve(NumVertices);
	m_indices.reserve(NumIndices);
}

void MeshAsset::InitAllMeshes(const aiScene* pScene) {
	for (unsigned int i = 0; i < m_meshes.size(); i++) {
		const aiMesh* paiMesh = pScene->mMeshes[i];
		InitSingleMesh(paiMesh);
	}
}

void MeshAsset::InitSingleMesh(const aiMesh* paiMesh) {

	const aiVector3D zero3D(0.0f, 0.0f, 0.0f);

	//populate vertex attribute vectors
	for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
		const aiVector3D& pPos = paiMesh->mVertices[i];
		const aiVector3D& pNormal = paiMesh->mNormals[i];
		const aiVector3D& pTexCoord = paiMesh->HasTextureCoords(0) ? paiMesh->mTextureCoords[0][i] : zero3D;
		const aiVector3D& tangent = paiMesh->HasTangentsAndBitangents() ? paiMesh->mTangents[i] : zero3D;

		/* AABB GENERATION */
		if (m_aabb.max.x < pPos.x)
			m_aabb.max.x = pPos.x;

		if (m_aabb.max.y < pPos.y)
			m_aabb.max.y = pPos.y;

		if (m_aabb.max.z < pPos.z)
			m_aabb.max.z = pPos.z;

		if (m_aabb.min.x > pPos.x)
			m_aabb.min.x = pPos.x;

		if (m_aabb.min.y > pPos.y)
			m_aabb.min.y = pPos.y;

		if (m_aabb.min.z > pPos.z)
			m_aabb.min.z = pPos.z;

		m_positions.push_back(glm::fvec3(pPos.x, pPos.y, pPos.z));
		m_normals.push_back(glm::fvec3(pNormal.x, pNormal.y, pNormal.z));
		m_texCoords.push_back(glm::fvec2(pTexCoord.x, pTexCoord.y));
		m_tangents.push_back(glm::fvec3(tangent.x, tangent.y, tangent.z));
	}

	for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
		const aiFace& face = paiMesh->mFaces[i];
		ASSERT(face.mNumIndices == 3);
		m_indices.push_back(face.mIndices[0]);
		m_indices.push_back(face.mIndices[1]);
		m_indices.push_back(face.mIndices[2]);
	}

}


void MeshAsset::PopulateBuffers() {
	GLCall(glBindVertexArray(m_VAO));

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

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_buffers[TANGENT_VB]));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(m_tangents[0]) * m_tangents.size(), &m_tangents[0], GL_STATIC_DRAW));
	GLCall(glEnableVertexAttribArray(TANGENT_LOCATION));
	GLCall(glVertexAttribPointer(TANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0));

	GLCall(glBindVertexArray(m_aabb_vao));

	m_bounding_box_vertices = {
		// -X
		glm::vec3(m_aabb.min.x, m_aabb.min.y, m_aabb.min.z),
		glm::vec3(m_aabb.min.x, m_aabb.min.y, m_aabb.max.z),
		glm::vec3(m_aabb.min.x, m_aabb.max.y, m_aabb.max.z),
		glm::vec3(m_aabb.min.x, m_aabb.max.y, m_aabb.min.z),

		// -Y
		glm::vec3(m_aabb.min.x, m_aabb.min.y, m_aabb.max.z),
		glm::vec3(m_aabb.min.x, m_aabb.min.y, m_aabb.min.z),
		glm::vec3(m_aabb.max.x, m_aabb.min.y, m_aabb.min.z),
		glm::vec3(m_aabb.max.x, m_aabb.min.y, m_aabb.max.z),

		// -Z
		glm::vec3(m_aabb.min.x, m_aabb.min.y, m_aabb.min.z),
		glm::vec3(m_aabb.min.x, m_aabb.max.y, m_aabb.min.z),
		glm::vec3(m_aabb.max.x, m_aabb.max.y, m_aabb.min.z),
		glm::vec3(m_aabb.max.x, m_aabb.min.y, m_aabb.min.z),

		// +X
		glm::vec3(m_aabb.max.x, m_aabb.min.y, m_aabb.max.z),
		glm::vec3(m_aabb.max.x, m_aabb.min.y, m_aabb.min.z),
		glm::vec3(m_aabb.max.x, m_aabb.max.y, m_aabb.min.z),
		glm::vec3(m_aabb.max.x, m_aabb.max.y, m_aabb.max.z),

		// +Y
		glm::vec3(m_aabb.min.x, m_aabb.max.y, m_aabb.max.z),
		glm::vec3(m_aabb.max.x, m_aabb.max.y, m_aabb.max.z),
		glm::vec3(m_aabb.max.x, m_aabb.max.y, m_aabb.min.z),
		glm::vec3(m_aabb.min.x, m_aabb.max.y, m_aabb.min.z),

		// +Z
		glm::vec3(m_aabb.min.x, m_aabb.max.y, m_aabb.max.z),
		glm::vec3(m_aabb.min.x, m_aabb.min.y, m_aabb.max.z),
		glm::vec3(m_aabb.max.x, m_aabb.min.y, m_aabb.max.z),
		glm::vec3(m_aabb.max.x, m_aabb.max.y, m_aabb.max.z),

	};

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_aabb_pos_vb));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 24, &m_bounding_box_vertices[0], GL_STATIC_DRAW));
	GLCall(glEnableVertexAttribArray(POSITION_LOCATION));
	GLCall(glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0));

	glBindVertexArray(0);
}

void MeshAsset::SubUpdateWorldTransformBuffer(unsigned int index_offset, const glm::mat4& transform) {
	GLCall(glBindVertexArray(m_VAO));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_buffers[WORLD_MAT_VB]));
	GLCall(glBufferSubData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * index_offset, sizeof(glm::mat4), &transform));


	GLCall(glBindVertexArray(0));
}

void MeshAsset::UpdateWorldTransformBuffer(const std::vector<glm::mat4>& transforms) {

	GLCall(glBindVertexArray(m_VAO));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_buffers[WORLD_MAT_VB]));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(transforms[0]) * transforms.size(), &transforms[0], GL_DYNAMIC_DRAW));

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
