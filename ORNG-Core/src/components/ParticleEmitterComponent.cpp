#include "components/ParticleEmitterComponent.h" 
#include "events/EventManager.h"

namespace ORNG {
	void ParticleEmitterComponent::DispatchUpdateEvent(EmitterSubEvent se, void* data_payload) {
		Events::ECS_Event<ParticleEmitterComponent> evt{ Events::ECS_EventType::COMP_UPDATED, this, se };
		evt.p_data = data_payload;

		Events::EventManager::DispatchEvent(evt);
	}
}