#pragma once

#include "rendering/VAO.h"
#include "Component.h"

namespace ORNG {
	// ParticleBufferComponent is used for programmable particle systems - they're just used to manage memory for particles that a layer can then run custom update/render functionality on
	// These can have particles assigned to them through EmitParticle(particle, m_buffer_id) in any shader as long as the particle append buffer is bound
	class ParticleBufferComponent : public Component {
		friend class ParticleSystem;
		friend class SceneSerializer;
		friend class EditorLayer;
	public:
		ParticleBufferComponent(SceneEntity* p_entity) : Component(p_entity) {};

		uint32_t GetBufferID() {
			return m_buffer_id;
		}

		unsigned GetMinAllocatedParticles() {
			return m_min_allocated_particles;
		}

		unsigned GetCurrentAllocatedParticles() {
			return m_current_allocated_particles;
		}

		SSBO<float> m_particle_ssbo{ false, GL_DYNAMIC_STORAGE_BIT };
	private:

		void DispatchUpdateEvent() {
			Events::ECS_Event<ParticleBufferComponent> e_event;
			e_event.affected_components[0] = this;
			e_event.event_type = Events::ECS_EventType::COMP_UPDATED;
			Events::EventManager::DispatchEvent(e_event);
		}

		// Used in EmitParticle calls in shaders to assign a particle to this specific buffer
		uint32_t m_buffer_id = 0;

		unsigned m_min_allocated_particles = 1'000'000;


		unsigned m_current_allocated_particles = 1'000'000;

	};

}