#ifndef AUDIOCOMP_H
#define AUDIOCOMP_H
#include "Component.h"
#include "events/EventManager.h"


namespace FMOD {
	class Channel;
}
struct FMOD_VECTOR;

namespace ORNG {
	class SoundAsset;

	class AudioComponent : public Component {
		friend class AudioSystem;
	public:
		explicit AudioComponent(SceneEntity* p_entity) : Component(p_entity) { };

		void Play(uint64_t uuid) {
			static uint64_t id = uuid;
			Events::ECS_Event<AudioComponent> e_event;
			e_event.event_type = Events::ECS_EventType::COMP_UPDATED;
			e_event.sub_event_type = (uint32_t)AudioEventType::PLAY;
			e_event.data_payload = id;
			e_event.affected_components[0] = this;

			Events::EventManager::DispatchEvent(e_event);
		}


		void Pause();

		enum class AudioEventType {
			PLAY,
			PAUSE
		};

	private:
		// Memory managed by AudioSystem
		FMOD_VECTOR* mp_fmod_pos = nullptr;
		FMOD_VECTOR* mp_fmod_vel = nullptr;

		FMOD::Channel* mp_channel = nullptr;
	};
}

#endif