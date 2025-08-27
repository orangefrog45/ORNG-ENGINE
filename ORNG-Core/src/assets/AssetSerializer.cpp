#include "pch/pch.h"
#include "assets/Prefab.h"
#include "assets/AssetSerializer.h"
#include "assets/AssetManager.h"
#include "assets/SoundAsset.h"
#include "core/GLStateManager.h"
#include "core/Window.h" // For shared loading context
#include "assets/SceneAsset.h"

#include <bitsery/adapter/stream.h>
#include <bitsery/traits/string.h>
#include <GLFW/glfw3.h> // For glfwmakecontextcurrent
#include <yaml-cpp/yaml.h>

using namespace ORNG;

constexpr size_t MAX_SCENE_YAML_SIZE = 100'000'000;

void AssetSerializer::ProcessAssetQueues() {
	for (int i = 0; i < m_mesh_loading_queue.size(); i++) {
		[[unlikely]] if (m_mesh_loading_queue[i].wait_for(std::chrono::nanoseconds(1)) == std::future_status::ready) {
			auto [result, p_mesh] = m_mesh_loading_queue[i].get();

			m_mesh_loading_queue.erase(m_mesh_loading_queue.begin() + i);
			i--;

			if (!result.has_value()) return;

			MeshLoadResult& result_val = result.value();
			MeshAssets assets = CreateAssetsFromMeshData(p_mesh, result_val);
			LoadMeshAssetIntoGL(assets.p_mesh);

			std::pair<MeshAssets*, MeshLoadResult*> payload = std::make_pair(&assets, &result_val);
			Events::AssetEvent e_event;
			e_event.event_type = Events::AssetEventType::MESH_LOADED;
			e_event.data_payload = reinterpret_cast<uint8_t*>(&payload);
			Events::EventManager::DispatchEvent(e_event);

			assets.p_mesh->ClearCPU_VertexData();
		}
	}
}

AssetSerializer::MeshAssets AssetSerializer::CreateAssetsFromMeshData(MeshAsset* p_mesh, MeshLoadResult& result) {
	MeshAssets assets;

	AssetManager::AddAsset(p_mesh);
	p_mesh->SetMeshData(result);

	assets.p_mesh = p_mesh;

	std::unordered_set<Texture2D*> invalid_textures;

	for (LoadedMeshTexture& tex : result.textures) {
		tex.p_tex->SetSpec(tex.spec);
		if (tex.p_tex->LoadFromBinary(tex.data.data(), tex.data.size(), false)) {
			AssetManager::AddAsset(tex.p_tex);
			assets.textures.push_back(tex.p_tex);
		} else {
			ORNG_CORE_ERROR("Failed to load texture from mesh data '{}'", result.original_file_path);
			invalid_textures.insert(tex.p_tex);
		}
	}

	auto* p_replacement_tex = AssetManager::GetAsset<Texture2D>(static_cast<uint64_t>(BaseAssetIDs::WHITE_TEXTURE));
	for (Material* p_mat : result.materials) {
		if (invalid_textures.contains(p_mat->base_colour_texture)) p_mat->base_colour_texture = p_replacement_tex;
		if (invalid_textures.contains(p_mat->normal_map_texture)) p_mat->normal_map_texture = p_replacement_tex;
		if (invalid_textures.contains(p_mat->roughness_texture)) p_mat->roughness_texture = p_replacement_tex;
		if (invalid_textures.contains(p_mat->metallic_texture)) p_mat->metallic_texture = p_replacement_tex;
		if (invalid_textures.contains(p_mat->ao_texture)) p_mat->ao_texture = p_replacement_tex;
		if (invalid_textures.contains(p_mat->emissive_texture)) p_mat->emissive_texture = p_replacement_tex;
		if (invalid_textures.contains(p_mat->displacement_texture)) p_mat->displacement_texture = p_replacement_tex;

		AssetManager::AddAsset(p_mat);
		p_mesh->m_material_uuids.push_back(p_mat->uuid());
	}

	assets.materials = result.materials;
	std::ranges::for_each(invalid_textures, [](auto* p_tex) {delete p_tex;});

	return assets;
}

void AssetSerializer::LoadMeshAssetIntoGL(MeshAsset* p_asset) {
	if (p_asset->m_is_loaded) {
		ORNG_CORE_ERROR("Mesh '{0}' is already loaded", p_asset->filepath);
		return;
	}

	GL_StateManager::BindVAO(p_asset->GetVAO().GetHandle());

	// Get directory used for finding material textures
	std::string dir = GetFileDirectory(p_asset->filepath);


	p_asset->m_vao.FillBuffers();
	p_asset->m_is_loaded = true;
}

void AssetSerializer::LoadMeshAsset(MeshAsset* p_asset, const std::string& raw_mesh_filepath) {
	m_mesh_loading_queue.emplace_back(std::async(std::launch::async, [p_asset, raw_mesh_filepath] {
		return std::make_pair(std::move(MeshAsset::LoadMeshDataFromFile(raw_mesh_filepath)), p_asset);
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

void AssetSerializer::SerializeSceneAsset(SceneAsset &scene_asset, BufferSerializer &ser) {
	std::string content = ReadTextFile(scene_asset.filepath);
	if (content.empty()) {
		ORNG_CORE_ERROR("Failed to serialize scene asset, yaml contents could not be read from: '{}'", scene_asset.filepath);
		return;
	}

	ser.text1b(content, MAX_SCENE_YAML_SIZE);
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

	uint32_t num_textures, num_meshes, num_sounds, num_prefabs, num_materials, num_phys_materials, num_scenes;
	des.value4b(num_textures);
	des.value4b(num_meshes);
	des.value4b(num_sounds);
	des.value4b(num_prefabs);
	des.value4b(num_materials);
	des.value4b(num_phys_materials);
	des.value4b(num_scenes);

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
		p_mesh->ClearCPU_VertexData();
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

	for (uint32_t i = 0; i < num_scenes; i++) {
		auto* p_scene = new SceneAsset{""};

		std::string contents;
		des.text1b(contents, MAX_SCENE_YAML_SIZE);

		try {
			p_scene->node = YAML::Load(contents);
			p_scene->uuid = UUID{p_scene->node["SceneUUID"].as<uint64_t>()};
		} catch(std::exception& e) {
			ORNG_CORE_ERROR("Failed to deserialize scene: '{}'", e.what());
		}

		AssetManager::AddAsset(p_scene);
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
	auto scene_view = std::vector<SceneAsset*>();

	for (auto& [uuid, p_asset] : m_manager.m_assets) {
		if (p_asset->uuid() < static_cast<uint64_t>(BaseAssetIDs::NUM_BASE_ASSETS))
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
		else if (auto* p_casted = dynamic_cast<SceneAsset*>(p_asset))
			scene_view.push_back(p_casted);
	}

	// Begin layout with number of assets
	ser.value4b(static_cast<uint32_t>(texture_view.size()));
	ser.value4b(static_cast<uint32_t>(mesh_view.size()));
	ser.value4b(static_cast<uint32_t>(sound_view.size()));
	ser.value4b(static_cast<uint32_t>(prefab_view.size()));
	ser.value4b(static_cast<uint32_t>(mat_view.size()));
	ser.value4b(static_cast<uint32_t>(scene_view.size()));

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
	for (auto* p_scene : scene_view) {
		SerializeSceneAsset(*p_scene, ser);
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
	// Scenes are also overlooked as they're explicitly serialized in the editor
	for (auto* p_texture : m_manager.GetView<Texture2D>()) {
		SerializeAssetToBinaryFile(*p_texture, p_texture->filepath);
	}

	for (auto* p_mat : m_manager.GetView<Material>()) {
		SerializeAssetToBinaryFile(*p_mat, p_mat->filepath);
	}

	for (auto* p_sound : m_manager.GetView<SoundAsset>()) {
		SerializeAssetToBinaryFile(*p_sound, p_sound->filepath);
	}
}

void AssetSerializer::Init() {
	glfwWindowHint(GLFW_VISIBLE, 0);
	mp_loading_context = glfwCreateWindow(100, 100, "ASSET_LOADING_CONTEXT", nullptr, Window::GetGLFWwindow());
}

void AssetSerializer::IStallUntilMeshesLoaded() {
	while (!m_mesh_loading_queue.empty()) {
		ProcessAssetQueues();
		std::this_thread::yield();
	}
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
	} else if (extension == ".oscene") {
		LoadSceneAssetFromFile(rel_path);
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

void AssetSerializer::LoadSceneAssetFromFile(const std::string &rel_path) {
	auto* p_scene = new SceneAsset{rel_path};
	const std::string yaml = ReadTextFile(rel_path);

	try {
		p_scene->node = YAML::Load(yaml);
		p_scene->uuid = UUID{p_scene->node["SceneUUID"].as<uint64_t>()};
	} catch(std::exception& e) {
		ORNG_CORE_ERROR("Failed to load scene asset from file: '{}'\n'{}'", rel_path, e.what());
	}

	m_manager.AddAsset(p_scene);
}


void AssetSerializer::LoadAssetsFromProjectPath(const std::string& project_dir) {
	// Lower priorities are loaded first
	const std::unordered_map<std::string, unsigned> asset_extension_priorities = {
		{".otex", 0},
		{".omat", 1},
		{".omesh", 2},
		{".opmat", 3},
		{".osound", 4},
		{".opfb", 5},
		{".cpp", 6},
		{".dll", 6},
		{".oscene", 7},
	};

	std::vector<std::pair<std::string, unsigned>> asset_paths;

 	for (const auto& entry : std::filesystem::recursive_directory_iterator(project_dir + "/res/")) {
		std::string path = entry.path().generic_string();

 		const std::string extension = GetFileExtension(path);
		if (entry.is_directory() || !asset_extension_priorities.contains(extension))
			continue;

 		const unsigned priority = asset_extension_priorities.at(extension);
 		auto it = asset_paths.begin();
 		while (it != asset_paths.end()) {
 			if (it->second > priority) break;
 			++it;
 		}

		const std::string rel_path = path.substr(path.rfind("/res/") + 1);
 		asset_paths.insert(it, std::make_pair(rel_path, priority));
	}

	for (const auto& [path, _] : asset_paths) {
		LoadAsset(path);
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

