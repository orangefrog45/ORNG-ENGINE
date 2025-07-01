#include "pch/pch.h"
#include "assets/Prefab.h"
#include "assets/AssetSerializer.h"
#include "assets/AssetManager.h"
#include "assets/SoundAsset.h"
#include "core/GLStateManager.h"
#include "core/Window.h" // For shared loading context
#include "physics/Physics.h" // For initializing physx assets
#include "assets/PhysXMaterialAsset.h"

#include <bitsery/adapter/stream.h>
#include <GLFW/glfw3.h> // For glfwmakecontextcurrent
#include <yaml-cpp/yaml.h>

using namespace ORNG;


void AssetSerializer::ProcessAssetQueues() {
	for (int i = 0; i < m_mesh_loading_queue.size(); i++) {
		[[unlikely]] if (m_mesh_loading_queue[i].wait_for(std::chrono::nanoseconds(1)) == std::future_status::ready) {
			auto* p_mesh = m_mesh_loading_queue[i].get();

			m_mesh_loading_queue.erase(m_mesh_loading_queue.begin() + i);
			i--;

			LoadMeshAssetIntoGL(p_mesh);

			Events::AssetEvent e_event;
			e_event.event_type = Events::AssetEventType::MESH_LOADED;
			e_event.data_payload = reinterpret_cast<uint8_t*>(p_mesh);
			Events::EventManager::DispatchEvent(e_event);

			p_mesh->ClearCPU_VertexData();
		}
	}
}

void AssetSerializer::LoadMeshAssetIntoGL(MeshAsset* p_asset) {
	if (p_asset->m_is_loaded) {
		ORNG_CORE_ERROR("Mesh '{0}' is already loaded", p_asset->filepath);
		return;
	}

	GL_StateManager::BindVAO(p_asset->GetVAO().GetHandle());

	// Get directory used for finding material textures
	std::string dir = GetFileDirectory(p_asset->filepath);

	// p_scene will be nullptr if the mesh was loaded from a binary file, then default materials will be provided
	if (p_asset->p_scene) {
		for (unsigned int i = 0; i < p_asset->p_scene->mNumMaterials; i++) {
			p_asset->num_materials++;
			const aiMaterial* p_material = p_asset->p_scene->mMaterials[i];
			Material* p_new_material = m_manager.AddAsset(new Material());

			// Load material textures
			p_new_material->base_colour_texture = CreateMeshAssetTexture(p_asset->p_scene, dir, aiTextureType_BASE_COLOR, p_material);
			p_new_material->normal_map_texture = CreateMeshAssetTexture(p_asset->p_scene, dir, aiTextureType_NORMALS, p_material);
			p_new_material->roughness_texture = CreateMeshAssetTexture(p_asset->p_scene, dir, aiTextureType_DIFFUSE_ROUGHNESS, p_material);
			p_new_material->metallic_texture = CreateMeshAssetTexture(p_asset->p_scene, dir, aiTextureType_METALNESS, p_material);
			p_new_material->ao_texture = CreateMeshAssetTexture(p_asset->p_scene, dir, aiTextureType_AMBIENT_OCCLUSION, p_material);

			// Load material properties
			aiColor3D base_color(0.0f, 0.0f, 0.0f);
			if (p_material->Get(AI_MATKEY_BASE_COLOR, base_color) == aiReturn_SUCCESS) {
				p_new_material->base_colour.r = base_color.r;
				p_new_material->base_colour.g = base_color.g;
				p_new_material->base_colour.b = base_color.b;
			}

			float roughness;
			float metallic;

			if (p_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS)
				p_new_material->roughness = roughness;

			if (p_material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS)
				p_new_material->metallic = metallic;

			// Check if the material has had any properties actually set - if not then use the default material instead of creating a new one.
			if (!p_new_material->base_colour_texture && !p_new_material->normal_map_texture && !p_new_material->roughness_texture
				&& !p_new_material->metallic_texture && !p_new_material->ao_texture && p_new_material->roughness == 0.2f && p_new_material->metallic == 0.0f) {
				m_manager.DeleteAsset(p_new_material);
				p_asset->m_material_uuids.push_back(ORNG_BASE_MATERIAL_ID);
			}
			else {
				p_asset->m_material_uuids.push_back(p_new_material->uuid());
			}
		}
	}

	p_asset->OnLoadIntoGL();

	p_asset->m_is_loaded = true;
}

void AssetSerializer::LoadMeshAsset(MeshAsset* p_asset, const std::string& raw_mesh_filepath) {
	m_mesh_loading_queue.emplace_back(std::async(std::launch::async, [p_asset, raw_mesh_filepath] {
		p_asset->LoadMeshData(raw_mesh_filepath);
		return p_asset;
		}));
};

void AssetSerializer::LoadTexture2D(Texture2D* p_tex) {
	static std::mutex m;
	m_texture_loading_queue.push_back(std::async(std::launch::async, [this, p_tex] {
		m.lock();
		glfwMakeContextCurrent(mp_loading_context);
		p_tex->LoadFromFile();
		m_manager.DispatchAssetEvent(Events::AssetEventType::TEXTURE_LOADED, reinterpret_cast<uint8_t*>(p_tex));
		glfwMakeContextCurrent(nullptr);
		m.unlock();
		}));
}

bool AssetSerializer::TryFetchRawSoundData(SoundAsset& sound, std::vector<std::byte>& output) {
	if (FileExists(sound.source_filepath)) { // Data still on disk, read from this
		ReadBinaryFile(sound.source_filepath, output);
	}
	else if (FileExists(sound.filepath)) { // Sound has been previously serialized into a binary file and data resides there
		SoundAsset dummy{ "" };
		DeserializeAssetBinary(sound.filepath, dummy, &output);
	}

	return false;
}

bool AssetSerializer::TryFetchRawTextureData(Texture2D& tex, std::vector<std::byte>& output) {
	if (FileExists(tex.m_spec.filepath)) { // Texture data still on disk, read from this
		ReadBinaryFile(tex.m_spec.filepath, output);
		return true;
	}
	else if (FileExists(tex.filepath)) { // Texture has been previously serialized into a binary file and data resides there
		Texture2D dummy{ "", 0 };
		DeserializeAssetBinary(tex.filepath, tex, &output);
		return true;
	}

	return false;
}


void AssetSerializer::DeserializeAssetsFromBinaryPackage(const std::string& package_filepath) {
	std::ifstream s{ package_filepath, std::ios::binary | std::ios::ate };
	if (!s.is_open()) {
		ORNG_CORE_ERROR("Package file deserialization error: Cannot open {0} for reading", package_filepath);
		return;
	}

	std::vector<std::byte> file_data;

	ReadBinaryFile(package_filepath, file_data);

	BufferDeserializer des{ file_data.begin(), file_data.end() };

	uint32_t num_textures, num_meshes, num_sounds, num_prefabs, num_materials, num_phys_materials;
	des.value4b(num_textures);
	des.value4b(num_meshes);
	des.value4b(num_sounds);
	des.value4b(num_prefabs);
	des.value4b(num_materials);
	des.value4b(num_phys_materials);

	std::vector<std::byte> bin_data;

	for (uint32_t i = 0; i < num_textures; i++) {
		auto* p_tex = new Texture2D{""};

		DeserializeTexture2D(*p_tex, bin_data, des);
		p_tex->LoadFromBinary(bin_data.data(), bin_data.size(), false);
		m_manager.AddAsset(p_tex);
		bin_data.clear();
	}

	for (uint32_t i = 0; i < num_meshes; i++) {
		auto* p_mesh = new MeshAsset{""};

		DeserializeMeshAsset(*p_mesh, des);
		LoadMeshAssetIntoGL(p_mesh);
		m_manager.AddAsset(p_mesh);
	}

	for (uint32_t i = 0; i < num_sounds; i++) {
		auto* p_sound = new SoundAsset{""};

		DeserializeSoundAsset(*p_sound, bin_data, des);
		p_sound->CreateSoundFromBinary(bin_data);
		m_manager.AddAsset(p_sound);
		bin_data.clear();
	}

	for (uint32_t i = 0; i < num_prefabs; i++) {
		auto* p_prefab = new Prefab{""};

		des.object(*p_prefab);
		p_prefab->node = YAML::Load(p_prefab->serialized_content);
		m_manager.AddAsset(p_prefab);
	}

	for (uint32_t i = 0; i < num_materials; i++) {
		auto* p_mat = new Material{""};

		DeserializeMaterialAsset(*p_mat, des);
		m_manager.AddAsset(p_mat);
	}

	for (uint32_t i = 0; i < num_phys_materials; i++) {
		auto* p_mat = new PhysXMaterialAsset{""};

		// Init material
		p_mat->p_material = Physics::GetPhysics()->createMaterial(0.5, 0.5, 0.5);
		DeserializePhysxMaterialAsset(*p_mat, des);
		m_manager.AddAsset(p_mat);
	}
}

void AssetSerializer::CreateBinaryAssetPackage(const std::string& output_path) {
	std::vector<std::byte> ser_buffer;
	BufferSerializer ser{ ser_buffer };

	auto texture_view = std::vector<Texture2D*>();
	auto mesh_view = std::vector<MeshAsset*>();
	auto sound_view = std::vector<SoundAsset*>();
	auto prefab_view = std::vector<Prefab*>();
	auto mat_view = std::vector<Material*>();
	auto phys_mat_view = std::vector<PhysXMaterialAsset*>();

	for (auto& [uuid, p_asset] : m_manager.m_assets) {
		if (p_asset->uuid() < ORNG_NUM_BASE_ASSETS)
			continue;

		if (auto* p_casted = dynamic_cast<Texture2D*>(p_asset))
			texture_view.push_back(p_casted);
		else if (auto* p_casted = dynamic_cast<MeshAsset*>(p_asset))
			mesh_view.push_back(p_casted);
		else if (auto* p_casted = dynamic_cast<SoundAsset*>(p_asset))
			sound_view.push_back(p_casted);
		else if (auto* p_casted = dynamic_cast<Prefab*>(p_asset))
			prefab_view.push_back(p_casted);
		else if (auto* p_casted = dynamic_cast<Material*>(p_asset))
			mat_view.push_back(p_casted);
		else if (auto* p_casted = dynamic_cast<PhysXMaterialAsset*>(p_asset))
			phys_mat_view.push_back(p_casted);
	}

	// Begin layout with number of assets
	ser.value4b(static_cast<uint32_t>(texture_view.size()));
	ser.value4b(static_cast<uint32_t>(mesh_view.size()));
	ser.value4b(static_cast<uint32_t>(sound_view.size()));
	ser.value4b(static_cast<uint32_t>(prefab_view.size()));
	ser.value4b(static_cast<uint32_t>(mat_view.size()));
	ser.value4b(static_cast<uint32_t>(phys_mat_view.size()));

	for (auto* p_texture : texture_view) {
		SerializeTexture2D(*p_texture, ser);
	}
	for (auto* p_mesh : mesh_view) {
		ser.object(*p_mesh);
	}
	for (auto* p_sound : sound_view) {
		SerializeSoundAsset(*p_sound, ser);
	}
	for (auto* p_prefab : prefab_view) {
		ser.object(*p_prefab);
	}
	for (auto* p_mat : mat_view) {
		ser.object(*p_mat);
	}
	for (auto* p_mat : phys_mat_view) {
		ser.object(*p_mat);
	}


	std::ofstream s{ output_path, s.binary | s.trunc | s.out };
	if (!s.is_open()) {
		ORNG_CORE_ERROR("Binary serialization error: Cannot open {0} for writing", output_path);
		return;
	}
	s.write(reinterpret_cast<const char*>(ser_buffer.data()), ser_buffer.size());
	s.close();
}

void AssetSerializer::SerializeAssets(const std::string& output_path) {
	// Serialize all assets currently loaded into asset manager
	// Meshes and prefabs are overlooked here as they are serialized automatically upon being loaded into the engine
	for (auto* p_texture : m_manager.GetView<Texture2D>()) {
		SerializeAssetToBinaryFile(*p_texture, p_texture->filepath);
	}

	for (auto* p_mat : m_manager.GetView<Material>()) {
		SerializeAssetToBinaryFile(*p_mat, p_mat->filepath);
	}

	for (auto* p_sound : m_manager.GetView<SoundAsset>()) {
		SerializeAssetToBinaryFile(*p_sound, p_sound->filepath);
	}

	for (auto* p_mat : m_manager.GetView<PhysXMaterialAsset>()) {
		SerializeAssetToBinaryFile(*p_mat, p_mat->filepath);
	}
}

void AssetSerializer::Init() {
	glfwWindowHint(GLFW_VISIBLE, 0);
	mp_loading_context = glfwCreateWindow(100, 100, "ASSET_LOADING_CONTEXT", nullptr, Window::GetGLFWwindow());
}

void AssetSerializer::IStallUntilMeshesLoaded() {
	while (!m_mesh_loading_queue.empty()) {
		for (int i = 0; i < m_mesh_loading_queue.size(); i++) {
			[[unlikely]] if (m_mesh_loading_queue[i].wait_for(std::chrono::nanoseconds(1)) == std::future_status::ready) {
				auto* p_mesh_asset = m_mesh_loading_queue[i].get();

				m_mesh_loading_queue.erase(m_mesh_loading_queue.begin() + i);
				i--;

				LoadMeshAssetIntoGL(p_mesh_asset);
				for (auto& future : m_texture_loading_queue) {
					future.get();
				}
				m_texture_loading_queue.clear();
				m_manager.DispatchAssetEvent(Events::AssetEventType::MESH_LOADED, reinterpret_cast<uint8_t*>(p_mesh_asset));
			}
		}
	}
}

bool AssetSerializer::ProcessEmbeddedTexture(Texture2D* p_tex, const aiTexture* p_ai_tex) {
	int x, y, channels;
	size_t size = glm::max(p_ai_tex->mHeight, 1u) * p_ai_tex->mWidth;
	stbi_uc* p_data = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(p_ai_tex->pcData), size, &x, &y, &channels, 0);

	if (!p_data) {
		ORNG_CORE_ERROR("Failed to load embedded texture");
		return false;
	}

	if (!p_tex->LoadFromBinary(reinterpret_cast<std::byte*>(p_data), size, true, x, y, channels)) {
		ORNG_CORE_ERROR("Failed to load embedded texture");
		return false;
	}

	// Embedded texture must be immediately serialized to a file while the image data is still available

	std::ofstream s{ p_tex->filepath, s.binary | s.trunc | s.out };
	if (!s.is_open()) {
		ORNG_CORE_ERROR("Binary serialization error: Cannot open {0} for writing", p_tex->filepath);
		return false;
	}

	bitsery::Serializer<bitsery::OutputStreamAdapter> ser{ s };

	// Serializes the texture
	SerializeTexture2D(*p_tex, ser, reinterpret_cast<std::byte*>(p_ai_tex->pcData), size);
	stbi_image_free(p_data);
	return true;
}

Texture2D* AssetSerializer::CreateMeshAssetTexture(const aiScene* p_scene, const std::string& dir, const aiTextureType& type, const aiMaterial* p_material) {
	Texture2D* p_tex = nullptr;

	if (p_material->GetTextureCount(type) > 0) {
		aiString path;
		if (p_material->GetTexture(type, 0, &path, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS) {
			std::string filename = GetFilename(path.data);
			Texture2DSpec base_spec;
			base_spec.generate_mipmaps = true;
			base_spec.mag_filter = GL_LINEAR;
			base_spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
			base_spec.filepath = dir + "/" + filename;
			base_spec.srgb_space = type == aiTextureType_BASE_COLOR ? true : false;

			p_tex = new Texture2D{ filename };
			p_tex->SetSpec(base_spec);
			p_tex = m_manager.AddAsset(p_tex);
			filename = filename + "_" + std::to_string(p_tex->uuid());

			if (auto* p_ai_tex = p_scene->GetEmbeddedTexture(path.C_Str())) {
				StringReplace(filename, "*", "[E]_"); // * will appear if the texture is embedded but causes serialization errors with the filepath, so change here
				p_tex->filepath = "./res/textures/" + filename + ".otex";
				ProcessEmbeddedTexture(p_tex, p_ai_tex);
			}
			else {
				LoadTexture2D(p_tex);
			}
		}
	}

	return p_tex;
}

void AssetSerializer::LoadAsset(const std::string& rel_path) {
	const std::string extension = GetFileExtension(rel_path);

	if (extension == ".otex") {
		LoadTexture2DAssetFromFile(rel_path);
	} else if (extension == ".osound") {
		LoadAudioAssetFromFile(rel_path);
	} else if (extension == ".omat") {
		LoadMaterialAssetFromFile(rel_path);
	} else if (extension == ".omesh") {
		LoadMeshAssetFromFile(rel_path);
	} else if (extension == ".opfb") {
		LoadPrefabAssetFromFile(rel_path);
	} else if (extension == ".cpp" || extension == ".dll") {
		LoadScriptAssetFromFile(rel_path);
	} else if (extension == ".opmat") {
		LoadPhysxAssetFromFile(rel_path);
	}
};

void AssetSerializer::LoadTexture2DAssetFromFile(const std::string& rel_path) {
	if (rel_path.find("diffuse_prefilter") != std::string::npos) return;

	Texture2DSpec spec{};
	spec.min_filter = GL_LINEAR_MIPMAP_LINEAR;
	spec.mag_filter = GL_LINEAR;
	spec.generate_mipmaps = true;
	spec.storage_type = GL_UNSIGNED_BYTE;
	spec.filepath = rel_path;

	auto* p_tex = new Texture2D{rel_path};
	std::vector<std::byte> binary_data;

	DeserializeAssetBinary(rel_path, *p_tex, &binary_data);
	p_tex->filepath = rel_path;
	m_manager.AddAsset(p_tex);
	p_tex->LoadFromBinary(binary_data.data(), binary_data.size(), false);
	m_manager.DispatchAssetEvent(Events::AssetEventType::TEXTURE_LOADED, reinterpret_cast<uint8_t*>(p_tex));
};

void AssetSerializer::LoadMeshAssetFromFile(const std::string& rel_path) {
	auto* p_mesh = new MeshAsset(rel_path);
	DeserializeAssetBinary(rel_path, *p_mesh);
	p_mesh->filepath = rel_path;
	m_manager.AddAsset(p_mesh);
	LoadMeshAssetIntoGL(p_mesh);
};

void AssetSerializer::LoadAudioAssetFromFile(const std::string& rel_path) {
	auto* p_sound = new SoundAsset{rel_path};
	std::vector<std::byte> raw_sound_data;
	DeserializeAssetBinary(rel_path, *p_sound, &raw_sound_data);
	p_sound->filepath = rel_path;
	p_sound->CreateSoundFromBinary(raw_sound_data);
	m_manager.AddAsset(p_sound);
};

void AssetSerializer::LoadMaterialAssetFromFile(const std::string& rel_path) {
	auto* p_mat = new Material{rel_path};
	p_mat->filepath = rel_path;
	DeserializeAssetBinary(rel_path, *p_mat);
	m_manager.AddAsset(p_mat);
};

void AssetSerializer::LoadPrefabAssetFromFile(const std::string& rel_path) {
	auto* p_prefab = new Prefab{rel_path};
	DeserializeAssetBinary(rel_path, *p_prefab);
	p_prefab->filepath = rel_path;
	m_manager.AddAsset(p_prefab);
	p_prefab->node = YAML::Load(p_prefab->serialized_content);
#ifndef ORNG_EDITOR_LAYER
	p_prefab->serialized_content.clear();
#endif
};

void AssetSerializer::LoadScriptAssetFromFile(const std::string& rel_path) {
	if (rel_path.find("scripts/includes") != std::string::npos)
		return;

#ifndef ORNG_EDITOR_LAYER
	if (GetFileExtension(rel_path) != ".dll") return;
#ifdef NDEBUG
	if (path_string.find("debug") != std::string::npos)
		return;
#else
	if (path_string.find("release") != std::string::npos)
		return;
#endif
	std::string dll_path = "./" + path_string.substr(path_string.rfind("res/scripts"));
	std::string rel_path = "./res/scripts/src/" + ReplaceFileExtension(GetFilename(dll_path), ".cpp");

	ScriptSymbols symbols = ScriptingEngine::LoadScriptDll(dll_path, rel_path, ReplaceFileExtension(GetFilename(rel_path), ""));
#else
	if (GetFileExtension(rel_path) != ".cpp" || rel_path.find("scripts/src/") == std::string::npos) return;
	ScriptSymbols symbols = ScriptingEngine::GetSymbolsFromScriptCpp(rel_path);
#endif

	if (symbols.loaded) {
		auto* p_script = new ScriptAsset{rel_path, symbols};
		p_script->uuid = UUID{symbols.uuid};
		m_manager.AddAsset(p_script);
	}
};

void AssetSerializer::LoadPhysxAssetFromFile(const std::string& rel_path) {
	auto* p_mat = new PhysXMaterialAsset{rel_path};
	// Init material
	p_mat->p_material = Physics::GetPhysics()->createMaterial(0.5, 0.5, 0.5);
	DeserializeAssetBinary(rel_path, *p_mat);
	m_manager.AddAsset(p_mat);
};


void AssetSerializer::LoadAssetsFromProjectPath(const std::string& project_dir) {
	for (const auto& entry : std::filesystem::recursive_directory_iterator(project_dir + "/res/")) {
		std::string path = entry.path().generic_string();

		if (entry.is_directory())
			continue;

		const std::string rel_path = path.substr(path.rfind("/res/") + 1);
		LoadAsset(rel_path);
	}
}

void AssetSerializer::DeserializeMaterialAsset(Material& data, BufferDeserializer& des) {
	des.object(data.base_colour);
	des.value1b(data.render_group);
	des.value4b(data.roughness);
	des.value4b(data.metallic);
	des.value4b(data.ao);
	des.value4b(data.emissive_strength);
	uint64_t texid;
	des.value8b(texid);
	if (texid != 0) data.base_colour_texture = m_manager.GetAsset<Texture2D>(texid);
	des.value8b(texid);
	if (texid != 0) data.normal_map_texture = m_manager.GetAsset<Texture2D>(texid);
	des.value8b(texid);
	if (texid != 0) data.metallic_texture = m_manager.GetAsset<Texture2D>(texid);
	des.value8b(texid);
	if (texid != 0) data.roughness_texture = m_manager.GetAsset<Texture2D>(texid);
	des.value8b(texid);
	if (texid != 0) data.ao_texture = m_manager.GetAsset<Texture2D>(texid);
	des.value8b(texid);
	if (texid != 0) data.displacement_texture = m_manager.GetAsset<Texture2D>(texid);
	des.value8b(texid);
	if (texid != 0) data.emissive_texture = m_manager.GetAsset<Texture2D>(texid);

	des.value4b(data.parallax_layers);
	des.object(data.tile_scale);
	des.text1b(data.name, ORNG_MAX_NAME_SIZE);
	des.object(data.uuid);
	des.object(data.spritesheet_data);

	uint32_t flags;
	des.value4b(flags);
	data.flags = (MaterialFlags)flags;
	des.value4b(data.displacement_scale);
	des.value4b(data.alpha_cutoff);
}

void AssetSerializer::DeserializePhysxMaterialAsset(PhysXMaterialAsset& data, BufferDeserializer& des) {
	des.object(data.uuid);
	des.container1b(data.name, ORNG_MAX_FILEPATH_SIZE);

	float sf, df, r;

	des.value4b(sf);
	des.value4b(df);
	des.value4b(r);

	data.p_material->setDynamicFriction(df);
	data.p_material->setStaticFriction(sf);
	data.p_material->setRestitution(r);
}

