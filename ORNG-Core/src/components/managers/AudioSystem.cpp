#include "pch/pch.h"
#include <fmod.hpp>
#include "audio/AudioEngine.h"
#include "assets/AssetManager.h"
#include "components/systems/AudioSystem.h"
#include "components/CameraComponent.h"
#include "scene/SceneEntity.h"

namespace ORNG {
	static void OnAudioComponentAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<AudioComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	static void OnAudioComponentDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<AudioComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}

	void AudioSystem::OnLoad() {
		std::string name = std::to_string(GetSceneUUID());
		AudioEngine::GetSystem()->createChannelGroup(name.c_str(), &mp_channel_group);
		m_audio_listener.scene_id = GetSceneUUID();
		m_audio_listener.OnEvent = [this](const Events::ECS_Event<AudioComponent>& e_event) {
			using enum Events::ECS_EventType;
			if (e_event.event_type == COMP_UPDATED)
				OnAudioUpdateEvent(e_event);
			else if (e_event.event_type == COMP_DELETED)
				OnAudioDeleteEvent(e_event);
			else if (e_event.event_type == COMP_ADDED)
				OnAudioAddEvent(e_event);
			};

		m_transform_listener.scene_id = GetSceneUUID();
		m_transform_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& e_event) {
			OnTransformEvent(e_event);
			};


		Events::EventManager::RegisterListener(m_audio_listener);
		Events::EventManager::RegisterListener(m_transform_listener);

		auto& reg = mp_scene->GetRegistry();
		m_connections[0] = reg.on_construct<AudioComponent>().connect<&OnAudioComponentAdd>();
		m_connections[1] = reg.on_destroy<AudioComponent>().connect<&OnAudioComponentDestroy>();
	}


	void AudioSystem::OnAudioDeleteEvent(const Events::ECS_Event<AudioComponent>& e_event) {
		// Don't error check this as FMOD will clean this up if the channel is invalid.
		e_event.p_component->mp_channel->stop();

		delete e_event.p_component->mp_fmod_pos;
		delete e_event.p_component->mp_fmod_vel;
	}

	void AudioSystem::OnUnload() {
		mp_channel_group->release();
		Events::EventManager::DeregisterListener(m_audio_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_transform_listener.GetRegisterID());

		m_connections[0].release();
		m_connections[1].release();
	}


	void AudioSystem::OnUpdate() {
		auto& reg = mp_scene->GetRegistry();

		if (auto* p_active_cam = mp_scene->GetActiveCamera()) {
			auto& transform = *p_active_cam->GetEntity()->GetComponent<TransformComponent>();
			auto pos = transform.GetAbsPosition();
			static FMOD_VECTOR cam_pos;
			cam_pos = { pos.x, pos.y, pos.z };
			static FMOD_VECTOR cam_velocity;
			static FMOD_VECTOR forward;
			forward = { transform.forward.x, transform.forward.y, transform.forward.z };
			static FMOD_VECTOR up;
			up = { transform.up.x, transform.up.y, transform.up.z };
			cam_velocity = { 0, 0, 0 };
			ORNG_CALL_FMOD(AudioEngine::GetSystem()->set3DListenerAttributes(0, &cam_pos, &cam_velocity, &forward, &up));
		}

		ORNG_CALL_FMOD(AudioEngine::GetSystem()->update());
	}

	void AudioSystem::OnTransformEvent(const Events::ECS_Event<TransformComponent>& e_event) {
		if (auto* p_sound_comp = e_event.p_component->GetEntity()->GetComponent<AudioComponent>()) {
			auto pos = e_event.p_component->GetAbsPosition();
			*p_sound_comp->mp_fmod_pos = { pos.x, pos.y, pos.z };
			p_sound_comp->mp_channel->set3DAttributes(p_sound_comp->mp_fmod_pos, p_sound_comp->mp_fmod_vel);
		}
	}


	void AudioSystem::OnAudioAddEvent(const Events::ECS_Event<AudioComponent>& e_event) {
		auto pos = e_event.p_component->GetEntity()->GetComponent<TransformComponent>()->GetAbsPosition();
		e_event.p_component->mp_fmod_pos = new FMOD_VECTOR{ pos.x, pos.y, pos.z };
		e_event.p_component->mp_fmod_vel = new FMOD_VECTOR{ 0, 0, 0 };

		e_event.p_component->SetSoundAssetUUID(ORNG_BASE_SOUND_ID);

		auto* p_asset = AssetManager::GetAsset<SoundAsset>(ORNG_BASE_SOUND_ID);

		// Default initialize channel with the base sound
		ORNG_CALL_FMOD(AudioEngine::GetSystem()->playSound(p_asset->p_sound, mp_channel_group, true, &e_event.p_component->mp_channel));
	}

	void AudioSystem::OnAudioUpdateEvent(const Events::ECS_Event<AudioComponent>& e_event) {
		auto* p_sound_comp = e_event.p_component;
		switch (e_event.sub_event_type) {
			using enum AudioComponent::AudioEventType;
		case (uint32_t)PLAY:
		{
			uint64_t uuid = *reinterpret_cast<uint64_t*>(e_event.p_data);
			auto* p_asset = AssetManager::GetAsset<SoundAsset>(uuid);

			if (!p_asset)
				return;

			ORNG_CALL_FMOD(AudioEngine::GetSystem()->playSound(p_asset->p_sound, mp_channel_group, true, &e_event.p_component->mp_channel));
			ORNG_CALL_FMOD(p_sound_comp->mp_channel->set3DAttributes(p_sound_comp->mp_fmod_pos, p_sound_comp->mp_fmod_vel));
			ORNG_CALL_FMOD(p_sound_comp->mp_channel->setVolume(p_sound_comp->m_volume));
			ORNG_CALL_FMOD(p_sound_comp->mp_channel->set3DMinMaxDistance(p_sound_comp->m_range.min, p_sound_comp->m_range.max));
			ORNG_CALL_FMOD(p_sound_comp->mp_channel->setPitch(p_sound_comp->m_pitch));
			ORNG_CALL_FMOD(p_sound_comp->mp_channel->setMode(e_event.p_component->mode));
			ORNG_CALL_FMOD(p_sound_comp->mp_channel->setPaused(false));

			break;
		}
		}
	}
}