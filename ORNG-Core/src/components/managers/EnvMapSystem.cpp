#include "components/systems/EnvMapSystem.h"
#include "yaml-cpp/yaml.h"

using namespace ORNG;

void EnvMapSystem::OnLoad() {
    m_serialization_listener.OnEvent = [this](const SceneSerializationEvent& _event) {
        if (_event.event_type == SceneSerializationEvent::Type::SERIALIZING) {
            SerializeEnvMap(_event);
        } else {
            DeserializeEnvMap(_event);
        }
    };

    Events::EventManager::RegisterListener(m_serialization_listener);
}

void EnvMapSystem::LoadSkyboxFromHDRFile(const std::string& filepath, unsigned resolution) {
    if (!FileExists(filepath)) {
        ORNG_CORE_ERROR("Failed to load skybox from file '{}', file doesn't exist.", filepath);
        return;
    }

    skybox.m_hdr_tex_filepath = filepath;
    skybox.m_resolution = resolution;
    m_loader.LoadSkybox(skybox.m_hdr_tex_filepath, skybox, skybox.GetResolution(), skybox.using_env_map);
}

void EnvMapSystem::DeserializeEnvMap(const SceneSerializationEvent& _event) {
    auto skybox_node = (*_event.data.p_node)["Skybox"];
    const auto using_env_maps = skybox_node["IBL"].as<bool>();
    const auto res = skybox_node["Resolution"].as<unsigned>();
    m_loader.LoadSkybox(skybox_node["HDR filepath"].as<std::string>(), skybox, res, using_env_maps);
}

void EnvMapSystem::SerializeEnvMap(const SceneSerializationEvent& _event) {
    auto& out = *_event.data.p_emitter;

    out << YAML::Key << "Skybox" << YAML::BeginMap;
    out << YAML::Key << "HDR filepath" << YAML::Value << skybox.m_hdr_tex_filepath;
    out << YAML::Key << "IBL" << YAML::Value << skybox.using_env_map;
    out << YAML::Key << "Resolution" << YAML::Value << skybox.GetResolution();
    out << YAML::EndMap;
}

void EnvMapSystem::OnUnload() {
    Events::EventManager::DeregisterListener(m_serialization_listener.GetRegisterID());
}