#include "Component.h"


namespace ORNG {
	class ParticleEmitterComponent : public Component {
		friend class ParticleSystem;
		friend class EditorLayer;
	public:
		ParticleEmitterComponent(SceneEntity* p_entity) : Component(p_entity) { };
	private:
		// Index into the SSBO's in ParticleSystem
		unsigned m_particle_start_index = 0;
		unsigned m_index = 0;
		unsigned m_num_particles = 64'000;
	};
}