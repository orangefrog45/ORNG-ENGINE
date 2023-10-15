#include "pch/pch.h"
#include "components/AudioComponent.h"
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

	bool AudioComponent::IsPaused() {
		bool b;
		mp_channel->getPaused(&b);
		return b;
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