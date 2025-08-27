#include "pch/pch.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#include <assimp/scene.h>
#include <assimp/postprocess.h>

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "rendering/MeshAsset.h"
#include "util/util.h"
#include "util/Log.h"
#include "util/TimeStep.h"


using namespace ORNG;

std::optional<MeshLoadResult> MeshAsset::LoadMeshDataFromFile(const std::string& raw_mesh_filepath) {
	MeshLoadResult result;
	result.original_file_path = raw_mesh_filepath;

	Assimp::Importer importer;
	ORNG_CORE_INFO("Loading mesh: {0}", raw_mesh_filepath);

	const auto time = TimeStep{TimeStep::TimeUnits::MILLISECONDS};
	const aiScene* p_scene = importer.ReadFile(raw_mesh_filepath.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace
		| aiProcess_ImproveCacheLocality);

	if (!p_scene || !InitFromScene(p_scene, result)) {
		ORNG_CORE_ERROR("Error parsing '{0}' : '{1}'", raw_mesh_filepath.c_str(), importer.GetErrorString());
		return std::nullopt;
	}

	unsigned num_verts = 0;
	unsigned num_indices = 0;
	CountVerticesAndIndices(p_scene, num_verts, num_indices, result);
	LoadMaterialsAndTextures(result, GetFileDirectory(raw_mesh_filepath), p_scene);

	ORNG_CORE_INFO("Mesh loaded in {0}ms: {1}", time.GetTimeInterval(), raw_mesh_filepath);

	importer.FreeScene();

	return result;
}


bool MeshAsset::InitFromScene(const aiScene* p_scene, MeshLoadResult& result) {
	result.submeshes.resize(p_scene->mNumMeshes);

	unsigned int num_vertices = 0;
	unsigned int num_indices = 0;

	CountVerticesAndIndices(p_scene, num_vertices, num_indices, result);

	if (num_vertices > ORNG_MAX_MESH_INDICES || num_indices > ORNG_MAX_MESH_INDICES) {
		ORNG_CORE_ERROR("Mesh asset exceeds maximum number of vertices, limit is {}", ORNG_MAX_MESH_INDICES);
		delete p_scene;
		return false;
	}

	result.vertex_data.positions.resize(num_vertices * 3);
	result.vertex_data.normals.resize(num_vertices * 3);
	result.vertex_data.tangents.resize(num_vertices * 3);

	result.vertex_data.tex_coords.reserve(num_vertices * 2);
	result.vertex_data.indices.reserve(num_indices);

	InitAllMeshes(p_scene, result);

	return true;
}

void MeshAsset::CountVerticesAndIndices(const aiScene* pScene, unsigned int& num_vertices, unsigned int& num_indices, MeshLoadResult& result) {
	for (unsigned int i = 0; i < result.submeshes.size(); i++) {
		result.submeshes[i].material_index = pScene->mMeshes[i]->mMaterialIndex;
		result.submeshes[i].num_indices = pScene->mMeshes[i]->mNumFaces * 3;
		result.submeshes[i].base_vertex = num_vertices;
		result.submeshes[i].base_index = num_indices;

		num_vertices += pScene->mMeshes[i]->mNumVertices;
		num_indices += result.submeshes[i].num_indices;
	}
}

void MeshAsset::InitAllMeshes(const aiScene* pScene, MeshLoadResult& result) {
	unsigned current_idx = 0;
	unsigned current_vertex = 0;

	for (unsigned int i = 0; i < result.submeshes.size(); i++) {
		const aiMesh* p_ai_mesh = pScene->mMeshes[i];
		InitSingleMesh(p_ai_mesh, current_idx, current_vertex, result);
		current_idx += p_ai_mesh->mNumFaces * 3;
		current_vertex += p_ai_mesh->mNumVertices;
	}
}

void MeshAsset::InitSingleMesh(const aiMesh* p_ai_mesh, unsigned current_idx, unsigned current_vertex, MeshLoadResult& result) {
	const aiVector3D zero3D{ 0.0f, 0.0f, 0.0f };

	memcpy(reinterpret_cast<std::byte*>(result.vertex_data.positions.data()) + current_vertex * sizeof(glm::vec3),
		p_ai_mesh->mVertices, p_ai_mesh->mNumVertices * sizeof(glm::vec3));
	memcpy(reinterpret_cast<std::byte*>(result.vertex_data.normals.data()) + current_vertex * sizeof(glm::vec3),
		p_ai_mesh->mNormals, p_ai_mesh->mNumVertices * sizeof(glm::vec3));
	memcpy(reinterpret_cast<std::byte*>(result.vertex_data.tangents.data()) + current_vertex * sizeof(glm::vec3),
		p_ai_mesh->mTangents, p_ai_mesh->mNumVertices * sizeof(glm::vec3));

	for (unsigned int i = 0; i < p_ai_mesh->mNumVertices; i++) {
		const aiVector3D& pos = p_ai_mesh->mVertices[i];
		const aiVector3D& tex_coord = p_ai_mesh->HasTextureCoords(0) ? p_ai_mesh->mTextureCoords[0][i] : zero3D;

		// AABB generation
		result.aabb.extents.x = glm::max(result.aabb.extents.x, abs(pos.x));
		result.aabb.extents.y = glm::max(result.aabb.extents.y, abs(pos.y));
		result.aabb.extents.z = glm::max(result.aabb.extents.z, abs(pos.z));

		result.vertex_data.tex_coords.push_back(tex_coord.x);
		result.vertex_data.tex_coords.push_back(tex_coord.y);
	}

	for (unsigned int i = 0; i < p_ai_mesh->mNumFaces; i++) {
		const aiFace& face = p_ai_mesh->mFaces[i];
		result.vertex_data.indices.push_back(face.mIndices[0]);
		result.vertex_data.indices.push_back(face.mIndices[1]);
		result.vertex_data.indices.push_back(face.mIndices[2]);
	}
}

LoadedMeshTexture* MeshAsset::CreateOrGetMaterialTexture(const std::string& dir, const aiTextureType& type, const aiMaterial* p_material, MeshLoadResult& result, const aiScene* p_scene) {
	LoadedMeshTexture ret{};
	bool success = false;
	aiString path;

	if (p_material->GetTextureCount(type) > 0) {
		unsigned uv_idx;
		if (p_material->GetTexture(type, 0, &path, nullptr, &uv_idx, nullptr, nullptr, nullptr) == aiReturn_SUCCESS) {
			// If texture has already been loaded, grab it here
			if (result.texture_name_lookup.contains(path.C_Str())) {
				return &result.textures[result.texture_name_lookup[path.C_Str()]];
			}

			std::string filename = GetFilename(path.data);
			ret.spec.generate_mipmaps = true;
			ret.spec.mag_filter = GL_LINEAR;
			ret.spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
			ret.spec.filepath = dir + "/" + filename;
			ret.spec.srgb_space = type == aiTextureType_BASE_COLOR;

			ret.p_tex = new Texture2D{ filename };

			if (auto* p_ai_tex = p_scene->GetEmbeddedTexture(path.C_Str())) {
				int x, y, channels;
				size_t size = glm::max(p_ai_tex->mHeight, 1u) * p_ai_tex->mWidth;
				stbi_uc* p_data = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(p_ai_tex->pcData), size, &x, &y, &channels, 0);

				success = static_cast<bool>(p_data);

				if (success) {
					ret.data.resize(size);
					memcpy(ret.data.data(), p_data, size);
				}
			}
			else {
				success = ReadBinaryFile(ret.spec.filepath, ret.data);
			}
		}
	}

	if (!success) {
		ORNG_CORE_ERROR("Failed to load texture from '{}'", path.C_Str());
		if (ret.p_tex) delete ret.p_tex;
		ret.p_tex = nullptr;
		return nullptr;
	}

	result.textures.push_back(std::move(ret));
	result.texture_name_lookup[path.C_Str()] = result.textures.size() - 1;

	return &result.textures[result.textures.size() - 1];
}

void MeshAsset::SetMeshData(MeshLoadResult& result) {
	m_num_indices = result.num_indices;
	m_num_materials = result.materials.size();

	m_vao.vertex_data = std::move(result.vertex_data);
	m_submeshes = result.submeshes;
	m_aabb = result.aabb;
}

void MeshAsset::LoadMaterialsAndTextures(MeshLoadResult& result, const std::string& dir, const aiScene* p_scene) {
	for (unsigned int i = 0; i < p_scene->mNumMaterials; i++) {
		const aiMaterial* p_material = p_scene->mMaterials[i];
		Material* p_new_material = new Material();

		if (LoadedMeshTexture* p_tex = CreateOrGetMaterialTexture(dir, aiTextureType_BASE_COLOR, p_material, result, p_scene); p_tex) {
			p_new_material->base_colour_texture = p_tex->p_tex;
		}
		if (LoadedMeshTexture* p_tex = CreateOrGetMaterialTexture(dir, aiTextureType_NORMALS, p_material, result, p_scene); p_tex) {
			p_new_material->normal_map_texture = p_tex->p_tex;
		}
		if (LoadedMeshTexture* p_tex = CreateOrGetMaterialTexture(dir, aiTextureType_DIFFUSE_ROUGHNESS, p_material, result, p_scene); p_tex) {
			p_new_material->roughness_texture = p_tex->p_tex;
		}
		if (LoadedMeshTexture* p_tex = CreateOrGetMaterialTexture(dir, aiTextureType_METALNESS, p_material, result, p_scene); p_tex) {
			p_new_material->metallic_texture = p_tex->p_tex;
		}
		if (LoadedMeshTexture* p_tex = CreateOrGetMaterialTexture(dir, aiTextureType_AMBIENT_OCCLUSION, p_material, result, p_scene); p_tex) {
			p_new_material->ao_texture = p_tex->p_tex;
		}
		if (LoadedMeshTexture* p_tex = CreateOrGetMaterialTexture(dir, aiTextureType_EMISSIVE, p_material, result, p_scene); p_tex) {
			p_new_material->emissive_texture = p_tex->p_tex;
		}
		if (LoadedMeshTexture* p_tex = CreateOrGetMaterialTexture(dir, aiTextureType_DISPLACEMENT, p_material, result, p_scene); p_tex) {
			p_new_material->displacement_texture = p_tex->p_tex;
		}

		// Load material properties
		aiColor3D base_color{0.0f, 0.0f, 0.0f};
		if (p_material->Get(AI_MATKEY_BASE_COLOR, base_color) == aiReturn_SUCCESS) {
			p_new_material->base_colour.r = base_color.r;
			p_new_material->base_colour.g = base_color.g;
			p_new_material->base_colour.b = base_color.b;
		}

		{
			aiString name;
			p_new_material->name = p_material->Get(AI_MATKEY_NAME, name) ? name.C_Str() : "";
		}

		if (float roughness; p_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS)
			p_new_material->roughness = roughness;

		if (float metallic; p_material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS)
			p_new_material->metallic = metallic;

		result.materials.push_back(p_new_material);
		//m_material_uuids.push_back(p_new_material->uuid());
	}
}


