#pragma once
#include "Component.h"
#include "util/Interpolators.h"

namespace ORNG {
	class Material;
	class MeshAsset;

	struct ParticleMeshResources : public Component {
		explicit ParticleMeshResources(SceneEntity* p_entity) : Component(p_entity) {}

		MeshAsset* p_mesh = nullptr;
		std::vector<const Material*> materials;
	};

	struct ParticleBillboardResources : public Component {
		explicit ParticleBillboardResources(SceneEntity* p_entity) : Component(p_entity) {}
		Material* p_material = nullptr;
	};


	class ParticleEmitterComponent final : public Component {
		friend class ParticleSystem;
		friend class EditorLayer;
		friend class SceneRenderer;
		friend class SceneSerializer;
	public:
		enum EmitterType : uint8_t {
			BILLBOARD,
			MESH
		};

		explicit ParticleEmitterComponent(SceneEntity* p_entity) : Component(p_entity) {}
		ParticleEmitterComponent& operator=(const ParticleEmitterComponent&) = delete;
		ParticleEmitterComponent(const ParticleEmitterComponent&) = delete;
		~ParticleEmitterComponent() override = default;

		// Maximum of 100,000 particles per emitter
		void SetNbParticles(int num) {
			if (num > 100'000)
				throw std::exception("ParticleEmitterComponent::SetNbParticles failed, number provided larger than limit (100,000)");

			int dif = num - static_cast<int>(m_num_particles);
			m_num_particles = static_cast<unsigned>(num);

			DispatchUpdateEvent(NB_PARTICLES_CHANGED, &dif);
		}

		[[nodiscard]] unsigned GetNbParticles() const noexcept {
			return m_num_particles;
		}

		void SetSpawnExtents(glm::vec3 extents) {
			m_spawn_extents = extents;
			DispatchUpdateEvent();
		}

		[[nodiscard]] glm::vec3 GetSpawnExtents() const noexcept {
			return m_spawn_extents;
		}

		void SetVelocityScale(glm::vec2 min_max) {
			m_velocity_min_max_scalar = min_max;
			DispatchUpdateEvent();
		}

		[[nodiscard]] glm::vec2 GetVelocityScale() const noexcept {
			return m_velocity_min_max_scalar;
		}

		void SetParticleLifespan(float lifespan_ms) {
			m_particle_lifespan_ms = lifespan_ms;
			DispatchUpdateEvent(LIFESPAN_CHANGED);
		}

		[[nodiscard]] float GetParticleLifespan() const noexcept {
			return m_particle_lifespan_ms;
		}

		void SetParticleSpawnDelay(float delay_ms) {
			m_particle_spawn_delay_ms = delay_ms;
			DispatchUpdateEvent(SPAWN_DELAY_CHANGED);
		}

		[[nodiscard]] float GetSpawnDelay() const noexcept {
			return m_particle_spawn_delay_ms;
		}

		void SetSpread(float spread) {
			m_spread = spread;
			DispatchUpdateEvent();
		}

		[[nodiscard]] float GetSpread() const noexcept {
			return m_spread;
		}

		void SetActive(bool active) {
			m_active = active;
			DispatchUpdateEvent(ACTIVE_STATUS_CHANGED);
		}

		[[nodiscard]] bool IsActive() const noexcept {
			return m_active;
		}

		void SetAcceleration(glm::vec3 a) {
			acceleration = a;
			DispatchUpdateEvent();
		}

		[[nodiscard]] glm::vec3 GetAcceleration() const noexcept {
			return acceleration;
		}

		[[nodiscard]] EmitterType GetType() const noexcept {
			return m_type;
		}

		void SetType(EmitterType type) {
			m_type = type;
			DispatchUpdateEvent(VISUAL_TYPE_CHANGED);
		}

		bool AreAnyEmittedParticlesAlive();

		[[nodiscard]] unsigned GetParticleStartIdx() const noexcept {
			return m_particle_start_index;
		}

		static constexpr int BASE_NUM_PARTICLES = 64;
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
			ACTIVE_STATUS_CHANGED = 128
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

		float m_last_active_status_change_time = 0.f;

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
