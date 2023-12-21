#include "components/ParticleEmitterComponent.h" 
#include "events/EventManager.h"

namespace ORNG {
	void ParticleEmitterComponent::DispatchUpdateEvent(EmitterSubEvent se, std::any data_payload) {
		Events::ECS_Event<ParticleEmitterComponent> evt{ Events::ECS_EventType::COMP_UPDATED, this, se };
		evt.data_payload = data_payload;

		Events::EventManager::DispatchEvent(evt);
	}
}