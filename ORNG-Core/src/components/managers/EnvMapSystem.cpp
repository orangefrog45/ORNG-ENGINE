#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <yaml-cpp/yaml.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "components/systems/EnvMapSystem.h"

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
    if (!m_loader.LoadSkybox(skybox.m_hdr_tex_filepath, skybox, skybox.GetResolution(), skybox.using_env_map))
        skybox.using_env_map = false;
}

void EnvMapSystem::DeserializeEnvMap(const SceneSerializationEvent& _event) {
    auto skybox_node = (*_event.data.p_node)["Skybox"];
    const auto using_env_map = skybox_node["IBL"].as<bool>();
    const auto res = skybox_node["Resolution"].as<unsigned>();
    skybox.m_hdr_tex_filepath = skybox_node["HDR filepath"].as<std::string>();
    skybox.m_resolution = res;
    skybox.using_env_map = using_env_map;
    if (!m_loader.LoadSkybox(skybox.m_hdr_tex_filepath, skybox, res, using_env_map))
        skybox.using_env_map = false;
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
