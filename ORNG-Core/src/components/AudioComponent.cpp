#include "pch/pch.h"
#include "components/AudioComponent.h"
#include "events/EventManager.h"

namespace ORNG {
	static void DispatchUpdateEvent(AudioComponent::AudioEventType type, AudioComponent* p_comp) {
		Events::ECS_Event<AudioComponent> e_event;
		e_event.event_type = Events::ECS_EventType::COMP_UPDATED;
		e_event.sub_event_type = (uint32_t)type;
		e_event.affected_components[0] = p_comp;
		e_event.affected_entities[0] = p_comp->GetEntity();

		Events::EventManager::DispatchEvent(e_event);
	}


	void AudioComponent::SetSound(const SoundAsset* p_asset) {
		mp_asset = p_asset;
	}

	void AudioComponent::Play() {
		DispatchUpdateEvent(AudioEventType::PLAY, this);
	}

	void AudioComponent::Pause() {
		DispatchUpdateEvent(AudioEventType::PAUSE, this);
	}
}