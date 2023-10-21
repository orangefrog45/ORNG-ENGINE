#include "pch/pch.h"
#include <fmod.hpp>
#include <fmod_errors.h>
#include "components/ComponentSystems.h"
#include "audio/AudioEngine.h"
#include "assets/AssetManager.h"
#include "scene/SceneEntity.h"

namespace ORNG {
	static void OnAudioComponentAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<AudioComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}


	static void OnAudioComponentDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<AudioComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}


	void AudioSystem::OnLoad() {
		const char* name = std::to_string(GetSceneUUID()).c_str();
		AudioEngine::GetSystem()->createChannelGroup(name, &mp_channel_group);
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

		mp_registry->on_construct<AudioComponent>().connect<&OnAudioComponentAdd>();
		mp_registry->on_destroy<AudioComponent>().connect<&OnAudioComponentDestroy>();
	}


	void AudioSystem::OnAudioDeleteEvent(const Events::ECS_Event<AudioComponent>& e_event) {
		if (auto result = e_event.affected_components[0]->mp_channel->stop(); result != FMOD_OK) {
			ORNG_CORE_ERROR("Fmod channel stop error '{0}'", FMOD_ErrorString(result));
		}

		delete e_event.affected_components[0]->mp_fmod_pos;
		delete e_event.affected_components[0]->mp_fmod_vel;
	}

	void AudioSystem::OnUnload() {
		Events::EventManager::DeregisterListener(m_audio_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_transform_listener.GetRegisterID());
	}



	void AudioSystem::OnUpdate() {
		if (auto* p_active_cam = mp_registry->try_get<CameraComponent>(*mp_active_cam_id)) {
			auto& transform = mp_registry->get<TransformComponent>(*mp_active_cam_id);
			auto pos = transform.GetAbsoluteTransforms()[0];
			static FMOD_VECTOR cam_pos;
			cam_pos = { pos.x, pos.y, pos.z };
			static FMOD_VECTOR cam_velocity;
			static FMOD_VECTOR forward;
			forward = { transform.forward.x, transform.forward.y, transform.forward.z };
			static FMOD_VECTOR up;
			up = { transform.up.x, transform.up.y, transform.up.z };
			cam_velocity = { 0, 0, 0 };
			AudioEngine::GetSystem()->set3DListenerAttributes(0, &cam_pos, &cam_velocity, &forward, &up);
		}

		AudioEngine::GetSystem()->update();
	}

	void AudioSystem::OnTransformEvent(const Events::ECS_Event<TransformComponent>& e_event) {
		if (auto* p_sound_comp = e_event.affected_components[0]->GetEntity()->GetComponent<AudioComponent>()) {
			auto pos = e_event.affected_components[0]->GetAbsoluteTransforms()[0];
			*p_sound_comp->mp_fmod_pos = { pos.x, pos.y, pos.z };
			p_sound_comp->mp_channel->set3DAttributes(p_sound_comp->mp_fmod_pos, p_sound_comp->mp_fmod_vel);
		}
	}


	void AudioSystem::OnAudioAddEvent(const Events::ECS_Event<AudioComponent>& e_event) {
		auto pos = e_event.affected_components[0]->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
		e_event.affected_components[0]->mp_fmod_pos = new FMOD_VECTOR{ pos.x, pos.y, pos.z };
		e_event.affected_components[0]->mp_fmod_vel = new FMOD_VECTOR{ 0, 0, 0 };

		e_event.affected_components[0]->SetSoundAssetUUID(ORNG_BASE_SOUND_ID);

		auto* p_asset = AssetManager::GetAsset<SoundAsset>(ORNG_BASE_SOUND_ID);

		// Default initialize channel with the base sound
		if (auto result = AudioEngine::GetSystem()->playSound(p_asset->p_sound, mp_channel_group, true, &e_event.affected_components[0]->mp_channel); result != FMOD_OK)
			ORNG_CORE_ERROR("Error initializing audio component with base sound '{0}', '{1}'", p_asset->source_filepath, FMOD_ErrorString(result));
	}

	void AudioSystem::OnAudioUpdateEvent(const Events::ECS_Event<AudioComponent>& e_event) {
		auto* p_sound_comp = e_event.affected_components[0];
		switch (e_event.sub_event_type) {
			using enum AudioComponent::AudioEventType;
		case (uint32_t)PLAY:
		{
			uint64_t uuid = std::any_cast<uint64_t>(e_event.data_payload);
			auto* p_asset = AssetManager::GetAsset<SoundAsset>(uuid);

			if (!p_asset)
				return;

			if (auto result = AudioEngine::GetSystem()->playSound(p_asset->p_sound, mp_channel_group, true, &e_event.affected_components[0]->mp_channel); result != FMOD_OK)
				ORNG_CORE_ERROR("Error playing sound '{0}', '{1}'", p_asset->source_filepath, FMOD_ErrorString(result));

			p_sound_comp->mp_channel->set3DAttributes(p_sound_comp->mp_fmod_pos, p_sound_comp->mp_fmod_vel);
			p_sound_comp->mp_channel->setVolume(p_sound_comp->m_volume);
			p_sound_comp->mp_channel->set3DMinMaxDistance(p_sound_comp->m_range.min, p_sound_comp->m_range.max);
			p_sound_comp->mp_channel->setPitch(p_sound_comp->m_pitch);
			p_sound_comp->mp_channel->setMode(FMOD_DEFAULT | FMOD_3D | FMOD_LOOP_OFF | FMOD_3D_LINEARROLLOFF);
			p_sound_comp->mp_channel->setPaused(false);
			break;
		}
		}
	}
}