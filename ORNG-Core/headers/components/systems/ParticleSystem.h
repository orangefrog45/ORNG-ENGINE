#pragma once
#include "components/systems/ComponentSystem.h"
#include "scene/Scene.h"
#include "shaders/Shader.h"

namespace ORNG {
	class ParticleSystem : public ComponentSystem {
		friend class EditorLayer;
		friend class GBufferPass;
		friend class TransparencyPass;
	public:
		ParticleSystem(Scene* p_scene);
		void OnLoad() override;
		void OnUnload() override;
		void OnUpdate() override;

		inline static constexpr uint64_t GetSystemUUID() { return 1927773874672; }

	private:
		void InitEmitter(ParticleEmitterComponent* p_comp);
		void InitParticles(ParticleEmitterComponent* p_comp);
		void OnEmitterUpdate(const Events::ECS_Event<ParticleEmitterComponent>& e_event);
		void OnEmitterUpdate(ParticleEmitterComponent* p_comp);
		void OnEmitterDestroy(ParticleEmitterComponent* p_comp, unsigned dif = 0);

		void InitBuffer(ParticleBufferComponent* p_comp);
		void OnBufferDestroy(ParticleBufferComponent* p_comp);
		void OnBufferUpdate(ParticleBufferComponent* p_comp);

		void OnEmitterVisualTypeChange(ParticleEmitterComponent* p_comp);

		void UpdateEmitterBufferAtIndex(unsigned index);

		std::array<entt::connection, 4> m_connections;

		Events::ECS_EventListener<ParticleEmitterComponent> m_particle_listener;
		Events::ECS_EventListener<ParticleBufferComponent> m_particle_buffer_listener;

		Events::ECS_EventListener<TransformComponent> m_transform_listener;

		// Stored in order based on their m_particle_start_index
		std::vector<entt::entity> m_emitter_entities;

		// Total particles belonging to emitters, does not include those belonging to ParticleBufferComponents which are stored separately
		unsigned total_emitter_particles = 0;

		SSBO<float> m_particle_ssbo{ false, 0 };
		SSBO<float> m_emitter_ssbo{ false, GL_DYNAMIC_STORAGE_BIT };

		Shader m_particle_cs;
		ShaderVariants m_particle_initializer_cs;
	};


	struct BillboardInstanceGroup {
		unsigned num_instances = 0;
		Material* p_material = nullptr;

		// Transforms stored as two vec3's (pos, scale)
		SSBO<float> transform_ssbo;
	};
}