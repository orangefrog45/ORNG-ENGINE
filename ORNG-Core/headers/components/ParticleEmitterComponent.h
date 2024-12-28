#include "Component.h"
#include "util/Interpolators.h"

namespace ORNG {
	class Material;
	class MeshAsset;

	struct ParticleMeshResources : public Component {
		ParticleMeshResources(SceneEntity* p_entity) : Component(p_entity) {};

		MeshAsset* p_mesh = nullptr;
		std::vector<const Material*> materials;
	};

	struct ParticleBillboardResources : public Component {
		ParticleBillboardResources(SceneEntity* p_entity) : Component(p_entity) {};
		Material* p_material = nullptr;
	};


	class ParticleEmitterComponent : public Component {
		friend class ParticleSystem;
		friend class EditorLayer;
		friend class SceneRenderer;
		friend class SceneSerializer;
	public:
		enum EmitterType : uint8_t {
			BILLBOARD,
			MESH
		};
		ParticleEmitterComponent(SceneEntity* p_entity) : Component(p_entity) { };

		// Maximum of 100,000 particles per emitter
		void SetNbParticles(unsigned num) {
			if (num > 100'000)
				throw std::exception("ParticleEmitterComponent::SetNbParticles failed, number provided larger than limit (100,000)");

			int dif = num - m_num_particles;
			m_num_particles = num;

			DispatchUpdateEvent(NB_PARTICLES_CHANGED, &dif);
		}

		unsigned GetNbParticles() {
			return m_num_particles;
		}

		void SetSpawnExtents(glm::vec3 extents) {
			m_spawn_extents = extents;
			DispatchUpdateEvent();
		}

		glm::vec3 GetSpawnExtents() {
			return m_spawn_extents;
		}

		void SetVelocityScale(glm::vec2 min_max) {
			m_velocity_min_max_scalar = min_max;
			DispatchUpdateEvent();
		}

		glm::vec2 GetVelocityScale() {
			return m_velocity_min_max_scalar;
		}

		void SetParticleLifespan(float lifespan_ms) {
			m_particle_lifespan_ms = lifespan_ms;
			DispatchUpdateEvent(LIFESPAN_CHANGED);
		}

		float GetParticleLifespan() {
			return m_particle_lifespan_ms;
		}

		void SetParticleSpawnDelay(float delay_ms) {
			m_particle_spawn_delay_ms = delay_ms;
			DispatchUpdateEvent(SPAWN_DELAY_CHANGED);
		}

		float GetSpawnDelay() {
			return m_particle_spawn_delay_ms;
		}

		void SetSpread(float spread) {
			m_spread = spread;
			DispatchUpdateEvent();
		}

		float GetSpread() {
			return m_spread;
		}

		void SetActive(bool active) {
			m_active = active;
			DispatchUpdateEvent();
		}

		bool IsActive() {
			return m_active;
		}

		void SetAcceleration(glm::vec3 a) {
			acceleration = a;
			DispatchUpdateEvent();
		}

		glm::vec3 GetAcceleration() {
			return acceleration;
		}

		EmitterType GetType() {
			return m_type;
		}

		void SetType(EmitterType type) {
			m_type = type;
			DispatchUpdateEvent(VISUAL_TYPE_CHANGED);
		}

		inline static const int BASE_NUM_PARTICLES = 64;

	private:

		EmitterType m_type = BILLBOARD;

		enum EmitterSubEvent : uint8_t {
			DEFAULT= 0,
			NB_PARTICLES_CHANGED = 2,
			LIFESPAN_CHANGED = 4,
			SPAWN_DELAY_CHANGED = 8,
			VISUAL_TYPE_CHANGED = 16,
			MODIFIERS_CHANGED = 32,
			FULL_UPDATE = 64,
		};

		void DispatchUpdateEvent(EmitterSubEvent se = DEFAULT, void* data_payload = nullptr);
		// User-configurable parameters

		glm::vec3 acceleration = { 0, 0, 0 };
		glm::vec3 m_spawn_extents = glm::vec3(50, 0, 50);
		glm::vec2 m_velocity_min_max_scalar = { 0.1, 50.0 };
		float m_particle_lifespan_ms = 1000.f;
		float m_particle_spawn_delay_ms = 1000.f / 64.f;
		float m_spread = 1.0; // 1 = 360 degree spread, 0 = no spread
		bool m_active = true;

		InterpolatorV3 m_life_colour_interpolator{ {0, 1}, {0, 1}, {1, 1, 1}, {1, 1, 1} };
		InterpolatorV3 m_life_scale_interpolator{ {0, 1}, {0, 1}, {1, 1, 1}, {1, 1, 1} };
		InterpolatorV1 m_life_alpha_interpolator{ {0, 1}, {0, 1}, 1, 1 };


		// State

		// Index into the SSBO's in ParticleSystem
		unsigned m_particle_start_index = 0;
		unsigned m_index = 0;
		unsigned m_num_particles = BASE_NUM_PARTICLES;
	};


}