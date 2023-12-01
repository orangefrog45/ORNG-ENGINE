#include "Component.h"


namespace ORNG {
	class ParticleEmitterComponent : public Component {
		friend class ParticleSystem;
		friend class EditorLayer;
	public:
		ParticleEmitterComponent(SceneEntity* p_entity) : Component(p_entity) { };
	private:

		// User-configurable parameters

		glm::vec3 m_spawn_extents = glm::vec3(1, 0, 1);
		glm::vec2 m_velocity_min_max_scalar = { 0.1, 50.0 };
		float m_spread = 2.f * glm::pi<float>(); // radians


		//State
		 
		// Index into the SSBO's in ParticleSystem
		unsigned m_particle_start_index = 0;
		unsigned m_index = 0;
		unsigned m_num_particles = 64'000;
	};
}