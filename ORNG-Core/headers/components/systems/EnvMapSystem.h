#pragma once
#include "scene/SceneSerializer.h"
#include "rendering/EnvMapLoader.h"

#include "components/systems/ComponentSystem.h"
#include "scene/Skybox.h"

namespace ORNG {
    // Handles loading, serialization, and deserialization of the scene skybox
    class EnvMapSystem : public ComponentSystem {
    public:
        explicit EnvMapSystem(Scene* p_scene) : ComponentSystem(p_scene) {};
        virtual ~EnvMapSystem() = default;

        void OnLoad() final;

        void OnUnload() final;

        inline static constexpr uint64_t GetSystemUUID() { return 89273647238766545; }

        void LoadSkyboxFromHDRFile(const std::string& filepath, unsigned resolution);

        Skybox skybox;
    private:
        void SerializeEnvMap(const SceneSerializationEvent& _event);

        void DeserializeEnvMap(const SceneSerializationEvent& _event);

        EnvMapLoader m_loader;
        Events::EventListener<SceneSerializationEvent> m_serialization_listener;
    };
}
