#pragma once

#include "components/systems/ComponentSystem.h"
#include "components/AudioComponent.h"
#include "components/TransformComponent.h"
#include "scene/Scene.h"

namespace FMOD {
	class ChannelGroup;
}

namespace ORNG {
	class AudioSystem : public ComponentSystem {
	public:
		explicit AudioSystem(Scene* p_scene) : ComponentSystem(p_scene) {}
		~AudioSystem() override = default;
		void OnLoad() final;
		void OnUnload() final;
		void OnUpdate() final;
		inline static constexpr uint64_t GetSystemUUID() { return 8881736346456; }
	private:
		void OnAudioDeleteEvent(const Events::ECS_Event<AudioComponent>& e_event);
		void OnAudioUpdateEvent(const Events::ECS_Event<AudioComponent>& e_event);
		void OnAudioAddEvent(const Events::ECS_Event<AudioComponent>& e_event);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& e_event);

		std::array<entt::connection, 2> m_connections;

		Events::ECS_EventListener<AudioComponent> m_audio_listener;
		Events::ECS_EventListener<TransformComponent> m_transform_listener;

		FMOD::ChannelGroup* mp_channel_group = nullptr;

		// Points to memory in scene's "CameraSystem" to find active camera
		entt::entity* mp_active_cam_id;
	};
}
