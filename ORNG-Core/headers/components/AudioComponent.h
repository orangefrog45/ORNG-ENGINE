#ifndef AUDIOCOMP_H
#define AUDIOCOMP_H
#include "Component.h"
#include "events/EventManager.h"


namespace FMOD {
	class Channel;
}
struct FMOD_VECTOR;

namespace ORNG {
	constexpr uint64_t INVALID_SOUND_UUID = 0;

	class SoundAsset;

	class AudioComponent : public Component {
		friend class AudioSystem;
	public:
		AudioComponent() = delete;
		explicit AudioComponent(SceneEntity* p_entity) : Component(p_entity) { };

		template <typename T>
		void DispatchAudioEvent(T data, Events::ECS_EventType event_type, uint32_t sub_event_type) {
			Events::ECS_Event<AudioComponent> e_event;
			e_event.event_type = event_type;
			e_event.sub_event_type = sub_event_type;
			e_event.data_payload = data;
			e_event.affected_components[0] = this;

			Events::EventManager::DispatchEvent(e_event);
		}

		void SetSoundAssetUUID(uint64_t uuid) {
			m_sound_asset_uuid = uuid;
		}

		void Play(uint64_t uuid = INVALID_SOUND_UUID) {
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

		void Pause() {
			DispatchAudioEvent(0, Events::ECS_EventType::COMP_UPDATED, (uint32_t)AudioEventType::PAUSE);
		}

		void SetPitch(float p) {
			DispatchAudioEvent(p, Events::ECS_EventType::COMP_UPDATED, (uint32_t)AudioEventType::SET_PITCH);
		}

		void SetVolume(float v) {
			DispatchAudioEvent(v, Events::ECS_EventType::COMP_UPDATED, (uint32_t)AudioEventType::SET_VOLUME);
		}

		struct AudioRange {
			AudioRange(float t_min, float t_max) : min(t_min), max(t_max) {};
			float min = 0.f;
			float max = 100.f;
		};

		void SetMinMaxRange(float min, float max) {
			DispatchAudioEvent(AudioRange(min, max), Events::ECS_EventType::COMP_UPDATED, (uint32_t)AudioEventType::SET_RANGE);
		}

		enum class AudioEventType {
			PLAY,
			PAUSE,
			SET_PITCH,
			SET_VOLUME,
			SET_RANGE
		};

	private:
		uint64_t m_sound_asset_uuid = INVALID_SOUND_UUID;

		// Memory managed by AudioSystem
		FMOD_VECTOR* mp_fmod_pos = nullptr;
		FMOD_VECTOR* mp_fmod_vel = nullptr;

		FMOD::Channel* mp_channel = nullptr;
	};
}

#endif