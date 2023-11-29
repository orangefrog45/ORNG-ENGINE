#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "core/GLStateManager.h"
#include "rendering/Renderer.h"
#include "scene/SceneEntity.h"

namespace ORNG {
	inline static void OnParticleEmitterAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<ParticleEmitterComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	inline static void OnParticleEmitterDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<ParticleEmitterComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}

	enum ParticleCSVariants {
		DEFAULT,
		EMITTER_DELETE
	};

	ParticleSystem::ParticleSystem(entt::registry* p_registry, uint64_t scene_uuid) : ComponentSystem(p_registry, scene_uuid) {
		if (!mp_particle_cs) {
			mp_particle_cs = &Renderer::GetShaderLibrary().CreateShader("particle update");
			mp_particle_cs->AddStage(GL_COMPUTE_SHADER, "res/shaders/ParticleCS.glsl");
			mp_particle_cs->Init();
		}

		if (!mp_particle_initializer_cs) {
			mp_particle_initializer_cs = &Renderer::GetShaderLibrary().CreateShaderVariants("particle initializer");
			mp_particle_initializer_cs->SetPath(GL_COMPUTE_SHADER, "res/shaders/ParticleInitializerCS.glsl");
			mp_particle_initializer_cs->AddVariant(ParticleCSVariants::DEFAULT, {}, { "u_start_index", "u_emitter_index" });
			mp_particle_initializer_cs->AddVariant(ParticleCSVariants::EMITTER_DELETE, { "EMITTER_DELETION" }, { "u_emitter_index" });
		}
	};

	void ParticleSystem::OnLoad() {
		if (!m_transform_ssbo.IsInitialized())
			m_transform_ssbo.Init();

		if (!m_particle_ssbo.IsInitialized())
			m_particle_ssbo.Init();

		if (!m_emitter_ssbo.IsInitialized())
			m_emitter_ssbo.Init();

		m_transform_ssbo.data_type = GL_FLOAT;
		m_transform_ssbo.draw_type = GL_DYNAMIC_DRAW;

		m_particle_ssbo.data_type = GL_FLOAT;
		m_particle_ssbo.draw_type = GL_DYNAMIC_DRAW;

		m_emitter_ssbo.data_type = GL_FLOAT;
		m_emitter_ssbo.draw_type = GL_DYNAMIC_DRAW;

		GL_StateManager::BindSSBO(m_emitter_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_EMITTERS);
		GL_StateManager::BindSSBO(m_transform_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_TRANSFORMS);
		GL_StateManager::BindSSBO(m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);

		m_particle_listener.scene_id = m_scene_uuid;
		m_particle_listener.OnEvent = [this](const Events::ECS_Event<ParticleEmitterComponent>& e_event) {
			switch (e_event.event_type) {
				using enum Events::ECS_EventType;
			case COMP_ADDED:
				InitEmitter(e_event.affected_components[0]);
				break;
			case COMP_UPDATED:
				OnEmitterUpdate(e_event.affected_components[0]);
				break;
			case COMP_DELETED:
				OnEmitterDestroy(e_event.affected_components[0]);
				break;
			}
		};

		m_transform_listener.scene_id = m_scene_uuid;
		m_transform_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& e_event) {
			if (e_event.event_type == Events::ECS_EventType::COMP_UPDATED) {
				if (auto* p_emitter = e_event.affected_components[0]->GetEntity()->GetComponent<ParticleEmitterComponent>())
					OnEmitterUpdate(p_emitter);
			}
		};

		mp_registry->on_destroy<ParticleEmitterComponent>().connect<&OnParticleEmitterDestroy>();
		mp_registry->on_construct<ParticleEmitterComponent>().connect<&OnParticleEmitterAdd>();

		Events::EventManager::RegisterListener(m_particle_listener);
		Events::EventManager::RegisterListener(m_transform_listener);
	}

	constexpr unsigned particle_struct_size = sizeof(float) * 8;
	constexpr unsigned emitter_struct_size = sizeof(float) * 8;

	void ParticleSystem::InitEmitter(ParticleEmitterComponent* p_comp) {
		if (!m_emitter_entities.empty()) {
			auto& comp = mp_registry->get<ParticleEmitterComponent>(m_emitter_entities[m_emitter_entities.size() - 1]);
			p_comp->m_particle_start_index = comp.m_particle_start_index + comp.m_num_particles;
		}
		else {
			p_comp->m_particle_start_index = 0;
		}

		m_emitter_entities.push_back(p_comp->GetEnttHandle());
		p_comp->m_index = m_emitter_entities.size() - 1;

		unsigned prev_total = total_particles;
		total_particles += p_comp->m_num_particles;


		// Allocate memory in the SSBO's for the new particles
		m_transform_ssbo.Resize(total_particles * sizeof(glm::mat4));
		m_emitter_ssbo.Resize(m_emitter_entities.size() * emitter_struct_size);
		m_particle_ssbo.Resize(total_particles * particle_struct_size);

		// Initialize emitter
		auto pos = p_comp->GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
		std::vector<float> emitter_data;
		emitter_data.resize(8);

		emitter_data[0] = pos.x;
		emitter_data[1] = pos.y;
		emitter_data[2] = pos.z;
		emitter_data[3] = 0.f; // padding
		emitter_data[4] = p_comp->m_particle_start_index;
		emitter_data[5] = p_comp->m_num_particles;
		emitter_data[6] = 0.f; //padding
		emitter_data[7] = 0.f; //padding
		glNamedBufferSubData(m_emitter_ssbo.GetHandle(), (m_emitter_entities.size() - 1) * emitter_struct_size, emitter_struct_size, emitter_data.data());
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		// Initialize new particles in shader
		GL_StateManager::BindSSBO(m_emitter_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_EMITTERS);
		GL_StateManager::BindSSBO(m_transform_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_TRANSFORMS);
		GL_StateManager::BindSSBO(m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);

		mp_particle_initializer_cs->Activate(ParticleCSVariants::DEFAULT);
		mp_particle_initializer_cs->SetUniform("u_start_index", p_comp->m_particle_start_index);
		mp_particle_initializer_cs->SetUniform<unsigned>("u_emitter_index", m_emitter_entities.size() - 1);

		glDispatchCompute(glm::ceil((total_particles - prev_total) / 32.f), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	void ParticleSystem::UpdateEmitterBufferAtIndex(unsigned index) {
		auto& comp = mp_registry->get<ParticleEmitterComponent>(m_emitter_entities[index]);
		std::vector<float> emitter_data;
		emitter_data.resize(emitter_struct_size);

		unsigned i = 0;
		auto pos = comp.GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
		emitter_data[i++] = pos.x;
		emitter_data[i++] = pos.y;
		emitter_data[i++] = pos.z;
		emitter_data[i++] = 0.f; // padding
		emitter_data[i++] = comp.m_particle_start_index;
		emitter_data[i++] = comp.m_num_particles;
		emitter_data[i++] = 0.f; //padding
		emitter_data[i++] = 0.f; //padding

		glNamedBufferSubData(m_emitter_ssbo.GetHandle(), index * emitter_struct_size, emitter_struct_size, emitter_data.data());

	}

	void ParticleSystem::UpdateEmitterBufferFull() {
		m_emitter_ssbo.Resize(m_emitter_entities.size() * emitter_struct_size);

		std::vector<float> emitter_data;
		emitter_data.resize(emitter_struct_size * m_emitter_entities.size());

		int i = 0;
		for (auto ent : m_emitter_entities) {
			auto& comp = mp_registry->get<ParticleEmitterComponent>(ent);
			auto pos = comp.GetEntity()->GetComponent<TransformComponent>()->GetAbsoluteTransforms()[0];
			emitter_data[i++] = pos.x;
			emitter_data[i++] = pos.y;
			emitter_data[i++] = pos.z;
			emitter_data[i++] = 0.f; // padding
			emitter_data[i++] = comp.m_particle_start_index;
			emitter_data[i++] = comp.m_num_particles;
			emitter_data[i++] = 0.f; //padding
			emitter_data[i++] = 0.f; //padding
		}
		glNamedBufferSubData(m_emitter_ssbo.GetHandle(), 0, emitter_struct_size * m_emitter_entities.size(), emitter_data.data());
	}


	void ParticleSystem::OnEmitterUpdate(ParticleEmitterComponent* p_comp) {
		UpdateEmitterBufferAtIndex(p_comp->m_index);
	}

	void ParticleSystem::OnEmitterDestroy(ParticleEmitterComponent* p_comp) {

		// Adjust emitter indices to account for deletion (handled in shader)
		mp_particle_initializer_cs->Activate(ParticleCSVariants::EMITTER_DELETE);
		mp_particle_initializer_cs->SetUniform("u_emitter_index", p_comp->m_index);
		// TODO: Parallelize
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		auto it = std::ranges::find(m_emitter_entities, p_comp->GetEnttHandle());

		for (auto i = it + 1; i < m_emitter_entities.end(); i++) {
			auto& em = mp_registry->get<ParticleEmitterComponent>(*i);
			em.m_particle_start_index -= p_comp->m_num_particles;
			em.m_index--;
		}

		m_emitter_entities.erase(it);
		total_particles -= p_comp->m_num_particles;

		m_transform_ssbo.Erase(p_comp->m_particle_start_index * sizeof(glm::mat4), p_comp->m_num_particles * sizeof(glm::mat4));
		m_emitter_ssbo.Erase(p_comp->m_index * emitter_struct_size, emitter_struct_size);
		m_particle_ssbo.Erase(p_comp->m_particle_start_index * particle_struct_size, p_comp->m_num_particles * particle_struct_size);

		GL_StateManager::BindSSBO(m_emitter_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_EMITTERS);
		GL_StateManager::BindSSBO(m_transform_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_TRANSFORMS);
		GL_StateManager::BindSSBO(m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
	}

	void ParticleSystem::OnUnload() {
	}

	void ParticleSystem::OnUpdate() {
		GL_StateManager::BindSSBO(m_transform_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_TRANSFORMS);
		mp_particle_cs->ActivateProgram();
		// Each thread processes 4 particles
		glDispatchCompute(glm::ceil((total_particles / 32.f) / 4.f), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
}