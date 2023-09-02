#include "pch/pch.h"
#include <fmod.hpp>
#include <fmod_errors.h>
#include "components/ComponentSystems.h"
#include "audio/AudioEngine.h"
#include "core/AssetManager.h"
#include "scene/SceneEntity.h"

namespace ORNG {
	void AudioSystem::OnLoad() {
		const char* name = std::to_string(GetSceneUUID()).c_str();
		AudioEngine::GetSystem()->createChannelGroup(name, &mp_channel_group);


		m_audio_listener.OnEvent = [this](const Events::ECS_Event<AudioComponent>& e_event) {
			using enum Events::ECS_EventType;
			if (e_event.event_type == COMP_UPDATED)
				OnAudioUpdateEvent(e_event);
			else if (e_event.event_type == COMP_DELETED)
				OnAudioDeleteEvent(e_event);
		};

		m_transform_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& e_event) {
			if (auto* p_sound_comp = e_event.affected_components[0]->GetEntity()->GetComponent<AudioComponent>()) {
				// Update positional audio
			}
		};


		Events::EventManager::RegisterListener(m_audio_listener);
		Events::EventManager::RegisterListener(m_transform_listener);

	}


	void AudioSystem::OnAudioDeleteEvent(const Events::ECS_Event<AudioComponent>& e_event) {
		e_event.affected_components[0]->mp_channel->stop();
	}

	void AudioSystem::OnUnload() {
		Events::EventManager::DeregisterListener(m_audio_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_transform_listener.GetRegisterID());
	}

	void AudioSystem::OnAudioUpdateEvent(const Events::ECS_Event<AudioComponent>& e_event) {
		switch (e_event.sub_event_type) {
			using enum AudioEventType;
		case (uint32_t)PLAY:
			if (auto result = AudioEngine::GetSystem()->playSound(e_event.affected_components[0]->mp_asset->p_sound, mp_channel_group, false, &e_event.affected_components[0]->mp_channel); result != FMOD_OK)
				ORNG_CORE_ERROR("Error playing sound '{0}', '{1}'", e_event.affected_components[0]->mp_asset->filepath, FMOD_ErrorString(result));
			break;
		case (uint32_t)PAUSE:
			e_event.affected_components[0]->mp_channel->setPaused(true);
			break;
		}
	}
}