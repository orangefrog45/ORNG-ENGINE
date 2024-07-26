#include "pch/pch.h"

#include "rendering/MeshAsset.h"
#include "util/util.h"
#include "util/Log.h"
#include "util/TimeStep.h"
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace ORNG {


	bool MeshAsset::LoadMeshData() {

		if (m_is_loaded) {
			ORNG_CORE_TRACE("Mesh '{0}' is already loaded", filepath);
			return true;
		}

		ORNG_CORE_INFO("Loading mesh: {0}", filepath);
		mp_importer = std::make_unique<Assimp::Importer>();

		TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);
		bool ret = false;
		p_scene = mp_importer->ReadFile(filepath.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace
			| aiProcess_ImproveCacheLocality);

		if (p_scene) {
			ret = InitFromScene(p_scene);
		}
		else {
			ORNG_CORE_ERROR("Error parsing '{0}' : '{1}'", filepath.c_str(), mp_importer->GetErrorString());
			return false;
		}

		ORNG_CORE_INFO("Mesh loaded in {0}ms: {1}", time.GetTimeInterval(), filepath);
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

	void MeshAsset::ReserveSpace(unsigned int NumVertices, unsigned int NumIndices) {
		m_vao.vertex_data.positions.reserve(NumVertices);
		m_vao.vertex_data.normals.reserve(NumVertices);
		m_vao.vertex_data.tangents.reserve(NumVertices);
		m_vao.vertex_data.tex_coords.reserve(NumVertices);
		m_vao.vertex_data.indices.reserve(NumIndices);
	}

	void MeshAsset::InitAllMeshes(const aiScene* pScene) {
		for (unsigned int i = 0; i < m_submeshes.size(); i++) {
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
			if (m_aabb.extents.x < abs(pPos.x))
				m_aabb.extents.x = abs(pPos.x);

			if (m_aabb.extents.y < abs(pPos.y))
				m_aabb.extents.y = abs(pPos.y);

			if (m_aabb.extents.z < abs(pPos.z))
				m_aabb.extents.z = abs(pPos.z);

			m_vao.vertex_data.positions.push_back(pPos.x);
			m_vao.vertex_data.positions.push_back(pPos.y);
			m_vao.vertex_data.positions.push_back(pPos.z);
			m_vao.vertex_data.normals.push_back(pNormal.x);
			m_vao.vertex_data.normals.push_back(pNormal.y);
			m_vao.vertex_data.normals.push_back(pNormal.z);
			m_vao.vertex_data.tex_coords.push_back(pTexCoord.x);
			m_vao.vertex_data.tex_coords.push_back(pTexCoord.y);
			m_vao.vertex_data.tangents.push_back(tangent.x);
			m_vao.vertex_data.tangents.push_back(tangent.y);
			m_vao.vertex_data.tangents.push_back(tangent.z);

		}


		for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
			const aiFace& face = paiMesh->mFaces[i];
			ASSERT(face.mNumIndices == 3);
			m_vao.vertex_data.indices.push_back(face.mIndices[0]);
			m_vao.vertex_data.indices.push_back(face.mIndices[1]);
			m_vao.vertex_data.indices.push_back(face.mIndices[2]);
		}

	}

	void MeshAsset::OnLoadIntoGL() {
		PopulateBuffers();
		m_is_loaded = true;

		if (mp_importer && mp_importer->GetScene())
			mp_importer->FreeScene();
	}

	void MeshAsset::PopulateBuffers() {
		m_vao.FillBuffers();
	}

}

