#include "pch/pch.h"

#include "rendering/MeshAsset.h"
#include "util/util.h"
#include "util/Log.h"
#include "components/TransformComponent.h"
#include "util/TimeStep.h"

namespace ORNG {


	bool MeshAsset::LoadMeshData() {

		if (m_is_loaded) {
			ORNG_CORE_TRACE("Mesh '{0}' is already loaded", m_filename);
			return true;
		}

		ORNG_CORE_INFO("Loading mesh: {0}", m_filename);

		TimeStep time = TimeStep(TimeStep::TimeUnits::MILLISECONDS);

		bool ret = false;
		p_scene = importer.ReadFile(m_filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace
			| aiProcess_ImproveCacheLocality);

		if (p_scene) {
			ret = InitFromScene(p_scene);
		}
		else {
			ORNG_CORE_ERROR("Error parsing '{0}' : '{1}'", m_filename.c_str(), importer.GetErrorString());
			return false;
		}

		ORNG_CORE_INFO("Mesh loaded in {0}ms: {1}", time.GetTimeInterval(), m_filename);
		return ret;
	}


	bool MeshAsset::InitFromScene(const aiScene* pScene) {
		m_submeshes.resize(pScene->mNumMeshes);
		m_original_materials.resize(pScene->mNumMaterials);

		unsigned int num_vertices = 0;

		CountVerticesAndIndices(pScene, num_vertices, num_indices);

		ReserveSpace(num_vertices, num_indices);

		InitAllMeshes(pScene);

		return true;
	}

	void MeshAsset::CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices) {

		for (unsigned int i = 0; i < m_submeshes.size(); i++) {

			m_submeshes[i].material_index = pScene->mMeshes[i]->mMaterialIndex;
			m_submeshes[i].num_indices = pScene->mMeshes[i]->mNumFaces * 3;
			m_submeshes[i].base_vertex = NumVertices;
			m_submeshes[i].base_index = NumIndices;

			NumVertices += pScene->mMeshes[i]->mNumVertices;
			NumIndices += m_submeshes[i].num_indices;
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

		/* Add on AABB visual to end of positions, doesn't need any normals etc as rendered with flat colour shading */
		std::array<glm::vec3, 24> aabb_positions = {
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

		m_vao.vertex_data.positions.insert(m_vao.vertex_data.positions.end(), aabb_positions.begin(), aabb_positions.end());

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

			m_vao.vertex_data.positions.push_back(glm::vec3(pPos.x, pPos.y, pPos.z));
			m_vao.vertex_data.normals.push_back(glm::vec3(pNormal.x, pNormal.y, pNormal.z));
			m_vao.vertex_data.tex_coords.push_back(glm::vec2(pTexCoord.x, pTexCoord.y));
			m_vao.vertex_data.tangents.push_back(glm::vec3(tangent.x, tangent.y, tangent.z));


		}


		for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
			const aiFace& face = paiMesh->mFaces[i];
			ASSERT(face.mNumIndices == 3);
			m_vao.vertex_data.indices.push_back(face.mIndices[0]);
			m_vao.vertex_data.indices.push_back(face.mIndices[1]);
			m_vao.vertex_data.indices.push_back(face.mIndices[2]);
		}

	}


	void MeshAsset::PopulateBuffers() {
		m_vao.FillBuffers();
	}

}

