#include "components/ParticleEmitterComponent.h" 
#include "events/EventManager.h"
#include "core/FrameTiming.h"

namespace ORNG {
	void ParticleEmitterComponent::DispatchUpdateEvent(EmitterSubEvent se, void* data_payload) {
		Events::ECS_Event<ParticleEmitterComponent> evt{ Events::ECS_EventType::COMP_UPDATED, this, se };
		evt.p_data = data_payload;

		Events::EventManager::DispatchEvent(evt);
	}

	bool ParticleEmitterComponent::AreAnyEmittedParticlesAlive() {
		// Would use scene time here but in the editor when the scene is paused this makes previewing particles a pain
		float time_since_last_active = FrameTiming::GetTotalElapsedTime() - m_last_active_status_change_time;
		return m_active || !m_active && time_since_last_active < m_particle_lifespan_ms;
	}
}