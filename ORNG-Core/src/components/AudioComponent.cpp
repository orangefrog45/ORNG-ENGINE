#include "pch/pch.h"
#include "components/AudioComponent.h"
#include "events/EventManager.h"
#include <fmod.hpp>

namespace ORNG {
	void AudioComponent::SetPitch(float p) {
		m_pitch = p;
		mp_channel->setPitch(p);
	}

	void AudioComponent::SetVolume(float v) {
		m_volume = v;
		mp_channel->setVolume(v);
	}

	void AudioComponent::SetMinMaxRange(float min, float max) {
		m_range.min = min; m_range.max = max;
		mp_channel->set3DMinMaxDistance(min, max);
	}

	void AudioComponent::SetPaused(bool b) {
		mp_channel->setPaused(b);
	}

	bool AudioComponent::IsPlaying() {
		bool b;
		mp_channel->isPlaying(&b);
		return b;
	}

	void AudioComponent::Play(uint64_t uuid) {
		// Large event, pass off to audiosystem to handle
		if (uuid == INVALID_SOUND_UUID) {
			if (m_sound_asset_uuid == INVALID_SOUND_UUID) {
				ORNG_CORE_ERROR("AudioComponent::Play failed, component does not have a valid sound uuid");
				return;
			}
			DispatchAudioEvent(m_sound_asset_uuid, Events::ECS_EventType::COMP_UPDATED, (uint32_t)AudioEventType::PLAY);
		}
		else {
			DispatchAudioEvent(uuid, Events::ECS_EventType::COMP_UPDATED, (uint32_t)AudioEventType::PLAY);
			m_sound_asset_uuid = uuid;
		}
	}

	void AudioComponent::DispatchAudioEvent(std::any data, Events::ECS_EventType event_type, uint32_t sub_event_type) {
		Events::ECS_Event<AudioComponent> e_event{ event_type, this, sub_event_type };
		e_event.data_payload = data;

		Events::EventManager::DispatchEvent(e_event);
	}

	bool AudioComponent::IsPaused() {
		bool b;
		mp_channel->getPaused(&b);
		return b;
	}

	void AudioComponent::SetLooped(bool looped) {
		is_looped = looped;
		mp_channel->setLoopCount(looped ? -1 : 0);
		mp_channel->setMode(FMOD_DEFAULT | FMOD_3D | (looped ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) | FMOD_3D_LINEARROLLOFF);
	}

	void AudioComponent::SetPlaybackPosition(float time_in_ms) {
		mp_channel->setPosition(time_in_ms, FMOD_TIMEUNIT_MS);
	}

	unsigned int AudioComponent::GetPlaybackPosition() {
		unsigned int pos;
		mp_channel->getPosition(&pos, FMOD_TIMEUNIT_MS);
		return pos;
	}

	float AudioComponent::GetVolume() {
		return m_volume;
	};

	float AudioComponent::GetPitch() {
		return m_pitch;
	};

	AudioComponent::AudioRange AudioComponent::GetMinMaxRange() {
		return m_range;
	};
}