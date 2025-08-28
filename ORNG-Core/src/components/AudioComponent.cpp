#include "pch/pch.h"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif defined(MSVC)
#pragma warning( push )
#pragma warning( disable : 4505 ) // 'FMOD_ErrorString': unreferenced function with internal linkage has been removed
#endif
#include <fmod.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(MSVC)
#pragma warning( pop )
#endif

#include "components/AudioComponent.h"
#include "events/EventManager.h"
#include "audio/AudioEngine.h"


namespace ORNG {
	AudioComponent::AudioComponent(SceneEntity* p_entity) : Component(p_entity) { mode = FMOD_DEFAULT | FMOD_3D | FMOD_LOOP_OFF | FMOD_3D_LINEARROLLOFF; };

	void AudioComponent::SetPitch(float p) {
		m_pitch = p;
		ORNG_CALL_FMOD(mp_channel->setPitch(p));
	}

	void AudioComponent::SetVolume(float v) {
		m_volume = v;
		ORNG_CALL_FMOD(mp_channel->setVolume(v));
	}

	void AudioComponent::SetMinMaxRange(float min, float max) {
		m_range.min = min; 
		m_range.max = max;
		ORNG_CALL_FMOD(mp_channel->set3DMinMaxDistance(min, max));
	}

	void AudioComponent::SetPaused(bool b) {
		ORNG_CALL_FMOD(mp_channel->setPaused(b));
	}

	void AudioComponent::Set2D(bool b) {
		if (b) {
			mode = mode & ~uint32_t{FMOD_3D};
			mode = mode | uint32_t{FMOD_2D};
		}
		else {
			mode = mode & ~uint32_t{FMOD_2D};
			mode = mode | uint32_t{FMOD_3D};
		}

		ORNG_CALL_FMOD(mp_channel->setMode(mode));
	}

	bool AudioComponent::IsPlaying() {
		bool b = false;
		ORNG_CALL_FMOD(mp_channel->isPlaying(&b));
		return b;
	}

	void AudioComponent::Stop() {
		ORNG_CALL_FMOD(mp_channel->stop());
	}

	void AudioComponent::Play(uint64_t uuid) {
		// Large event, pass off to audiosystem to handle
		if (uuid == INVALID_SOUND_UUID) {
			if (m_sound_asset_uuid == INVALID_SOUND_UUID) {
				ORNG_CORE_ERROR("AudioComponent::Play failed, component does not have a valid sound uuid");
				return;
			}
			DispatchAudioEvent(&m_sound_asset_uuid, Events::ECS_EventType::COMP_UPDATED, static_cast<uint32_t>(AudioEventType::PLAY));
		}
		else {
			DispatchAudioEvent(&uuid, Events::ECS_EventType::COMP_UPDATED, static_cast<uint32_t>(AudioEventType::PLAY));
			m_sound_asset_uuid = uuid;
		}
	}

	void AudioComponent::DispatchAudioEvent(void* p_data, Events::ECS_EventType event_type, uint8_t sub_event_type) {
		Events::ECS_Event<AudioComponent> e_event{ event_type, this, sub_event_type };
		e_event.p_data = p_data;

		Events::EventManager::DispatchEvent(e_event);
	}

	bool AudioComponent::IsPaused() {
		bool b;
		mp_channel->getPaused(&b);
		return b;
	}

	void AudioComponent::SetLooped(bool looped) {
		is_looped = looped;
		ORNG_CALL_FMOD(mp_channel->setLoopCount(looped ? -1 : 0));
		mode = FMOD_DEFAULT | FMOD_3D | (looped ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) | FMOD_3D_LINEARROLLOFF;
		ORNG_CALL_FMOD(mp_channel->setMode(mode));
	}


	void AudioComponent::Set3DLevel(float level) {
		m_level_3d = level;
		ORNG_CALL_FMOD(mp_channel->set3DLevel(level));
	}

	void AudioComponent::SetPan(float pan) {
		m_pan = pan;
		ORNG_CALL_FMOD(mp_channel->setPan(pan));
	}


	void AudioComponent::SetPlaybackPosition(unsigned time_in_ms) {
		ORNG_CALL_FMOD(mp_channel->setPosition(time_in_ms, FMOD_TIMEUNIT_MS));
	}

	unsigned int AudioComponent::GetPlaybackPosition() {
		unsigned int pos;
		ORNG_CALL_FMOD(mp_channel->getPosition(&pos, FMOD_TIMEUNIT_MS));
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
