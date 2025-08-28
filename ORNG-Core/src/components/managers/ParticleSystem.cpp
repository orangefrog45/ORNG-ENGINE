#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "core/GLStateManager.h"
#include "rendering/Renderer.h"
#include "scene/SceneEntity.h"
#include "assets/AssetManager.h"
#include "components/ParticleBufferComponent.h"
#include "core/FrameTiming.h"
#include "components/systems/ParticleSystem.h"

#include <glm/glm/gtc/round.hpp>


namespace ORNG {
	constexpr unsigned particle_struct_size = sizeof(float) * 4 + sizeof(glm::vec4) * 4;
	constexpr unsigned emitter_struct_size = sizeof(float) * 36 + InterpolatorV3::GPU_STRUCT_SIZE_BYTES * 2 + sizeof(float) * 6 + InterpolatorV1::GPU_STRUCT_SIZE_BYTES + sizeof(float) * 3;

	inline static void OnParticleEmitterAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<ParticleEmitterComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	inline static void OnParticleEmitterDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<ParticleEmitterComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}


	inline static void OnParticleBufferAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<ParticleBufferComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	inline static void OnParticleBufferDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<ParticleBufferComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}

	enum ParticleCSVariants {
		DEFAULT,
		EMITTER_DELETE_DECREMENT_EMITTERS,
		EMITTER_DELETE_DECREMENT_PARTICLES,
		INITIALIZE_AS_DEAD,
		COPY_ALIVE_PARTICLES,
	};

	ParticleSystem::ParticleSystem(Scene* p_scene) : ComponentSystem(p_scene) {

	};

	void ParticleSystem::OnLoad() {
		m_particle_cs.AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/ParticleCS.glsl");
		m_particle_cs.Init();

		m_particle_initializer_cs.SetPath(GL_COMPUTE_SHADER, "res/core-res/shaders/ParticleInitializerCS.glsl");
		m_particle_initializer_cs.AddVariant(ParticleCSVariants::DEFAULT, {}, { "u_start_index", "u_emitter_index" });
		m_particle_initializer_cs.AddVariant(ParticleCSVariants::EMITTER_DELETE_DECREMENT_EMITTERS, { "EMITTER_DELETE", "EMITTER_DELETE_DECREMENT_EMITTERS" }, { "u_emitter_index", "u_num_emitters" });
		m_particle_initializer_cs.AddVariant(ParticleCSVariants::EMITTER_DELETE_DECREMENT_PARTICLES, { "EMITTER_DELETE", "EMITTER_DELETE_DECREMENT_PARTICLES" }, { "u_start_index", "u_num_particles" });
		m_particle_initializer_cs.AddVariant(ParticleCSVariants::INITIALIZE_AS_DEAD, { "INITIALIZE_AS_DEAD" }, { "u_start_index" });

		if (!m_particle_ssbo.IsInitialized()) {
			m_particle_ssbo.Init();
			m_emitter_ssbo.Init();
		}

		m_particle_ssbo.draw_type = GL_DYNAMIC_DRAW;
		m_emitter_ssbo.draw_type = GL_DYNAMIC_DRAW;

		GL_StateManager::BindSSBO(m_emitter_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_EMITTERS);
		GL_StateManager::BindSSBO(m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);

		m_particle_listener.scene_id = GetSceneUUID();
		m_particle_listener.OnEvent = [this](const Events::ECS_Event<ParticleEmitterComponent>& e_event) {
			switch (e_event.event_type) {
				using enum Events::ECS_EventType;
			case COMP_ADDED:
				InitEmitter(e_event.p_component);
				break;
			case COMP_UPDATED:
				OnEmitterUpdate(e_event);
				break;
			case COMP_DELETED:
				OnEmitterDestroy(e_event.p_component);
				break;
			}
			};

		m_particle_buffer_listener.scene_id = GetSceneUUID();
		m_particle_buffer_listener.OnEvent = [](const Events::ECS_Event<ParticleBufferComponent>& e_event) {
			switch (e_event.event_type) {
				using enum Events::ECS_EventType;
			case COMP_ADDED:
				//InitBuffer(e_event.p_component);
				break;
			case COMP_UPDATED:
				//OnBufferUpdate(e_event.p_component);
				break;
			case COMP_DELETED:
				break;
			}
			};

		m_transform_listener.scene_id = GetSceneUUID();
		m_transform_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& e_event) {
			if (e_event.event_type == Events::ECS_EventType::COMP_UPDATED) {
				if (auto* p_emitter = e_event.p_component->GetEntity()->GetComponent<ParticleEmitterComponent>())
					OnEmitterUpdate(p_emitter);
			}
			};

		auto& reg = mp_scene->GetRegistry();

		reg.on_destroy<ParticleEmitterComponent>().connect<&OnParticleEmitterDestroy>();
		reg.on_construct<ParticleEmitterComponent>().connect<&OnParticleEmitterAdd>();

		reg.on_destroy<ParticleBufferComponent>().connect<&OnParticleBufferDestroy>();
		reg.on_construct<ParticleBufferComponent>().connect<&OnParticleBufferAdd>();

		Events::EventManager::RegisterListener(m_particle_listener);
		Events::EventManager::RegisterListener(m_transform_listener);
		Events::EventManager::RegisterListener(m_particle_buffer_listener);
	}

	void ParticleSystem::InitParticles(ParticleEmitterComponent* p_comp) {
		GL_StateManager::BindSSBO(m_emitter_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_EMITTERS);
		GL_StateManager::BindSSBO(m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);

		m_particle_initializer_cs.Activate(ParticleCSVariants::DEFAULT);
		m_particle_initializer_cs.SetUniform("u_start_index", p_comp->m_particle_start_index);
		m_particle_initializer_cs.SetUniform("u_emitter_index", static_cast<unsigned>(m_emitter_entities.size() - 1));

		if (p_comp->m_num_particles > 0) {
			GL_StateManager::DispatchCompute(static_cast<int>(glm::ceil(static_cast<float>(p_comp->m_num_particles) / 32.f)), 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}
	}

	void ParticleSystem::InitEmitter(ParticleEmitterComponent* p_comp) {
		auto* p_ent = p_comp->GetEntity();
		auto* p_bb_res = p_ent->GetComponent<ParticleBillboardResources>();
		auto* p_mesh_res = p_ent->GetComponent<ParticleMeshResources>();


		if (p_comp->m_type == ParticleEmitterComponent::BILLBOARD && !p_bb_res) {
			p_bb_res = p_ent->AddComponent<ParticleBillboardResources>();
			p_bb_res->p_material = AssetManager::GetAsset<Material>(static_cast<uint64_t>(BaseAssetIDs::DEFAULT_MATERIAL));
		}
		else if (p_comp->m_type == ParticleEmitterComponent::MESH && !p_mesh_res) {
			p_mesh_res = p_ent->AddComponent<ParticleMeshResources>();
			p_mesh_res->materials = { AssetManager::GetAsset<Material>(static_cast<uint64_t>(BaseAssetIDs::DEFAULT_MATERIAL)) };
			p_mesh_res->p_mesh = AssetManager::GetAsset<MeshAsset>(static_cast<uint64_t>(BaseAssetIDs::CUBE_MESH));
		}

		if (!m_emitter_entities.empty()) {
			auto& comp = mp_scene->GetRegistry().get<ParticleEmitterComponent>(m_emitter_entities[m_emitter_entities.size() - 1]);
			p_comp->m_particle_start_index = comp.m_particle_start_index + comp.m_num_particles;
		}
		else {
			p_comp->m_particle_start_index = 0;
		}

		m_emitter_entities.push_back(p_comp->GetEnttHandle());
		p_comp->m_index = static_cast<unsigned>(m_emitter_entities.size() - 1);

		total_emitter_particles += p_comp->m_num_particles;

		if (m_particle_ssbo.GetGPU_BufferSize() <= static_cast<unsigned>(total_emitter_particles) * particle_struct_size ) {
			const unsigned new_allocated_particles = glm::ceilMultiple(glm::max(static_cast<unsigned>(total_emitter_particles * 3 / 2), 10000u), 64u);
			m_particle_ssbo.Resize(new_allocated_particles * particle_struct_size);
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

			m_particle_initializer_cs.Activate(ParticleCSVariants::INITIALIZE_AS_DEAD);
			m_particle_initializer_cs.SetUniform("u_start_index", p_comp->m_particle_start_index + p_comp->m_num_particles);
			GL_StateManager::DispatchCompute(static_cast<int>(glm::ceil(static_cast<float>(new_allocated_particles - (p_comp->m_particle_start_index + p_comp->m_num_particles)) / 32.f)), 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		if (m_emitter_ssbo.GetGPU_BufferSize() < m_emitter_entities.size() * emitter_struct_size) {
			m_emitter_ssbo.Resize(glm::max(static_cast<unsigned>(m_emitter_entities.size() * 3u) / 2u, 10u) * emitter_struct_size);
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		}

		UpdateEmitterBufferAtIndex(p_comp->m_index);

		// Initialize new particles in shader
		InitParticles(p_comp);
	}

	void ParticleSystem::UpdateEmitterBufferAtIndex(unsigned index) {
		auto& comp = mp_scene->GetRegistry().get<ParticleEmitterComponent>(m_emitter_entities[index]);
		auto* p_transform = comp.GetEntity()->GetComponent<TransformComponent>();

		std::array<std::byte, emitter_struct_size> emitter_data;
		std::byte* p_byte = &emitter_data[0];

		ConvertToBytes(p_byte,
			p_transform->GetAbsPosition(), 0.0f,
			comp.m_particle_start_index, 
			comp.m_num_particles, 
			comp.m_spread * 2.0f * glm::pi<float>(),
			comp.m_velocity_min_max_scalar.x,
			glm::mat4_cast(p_transform->GetAbsOrientationQuat()),
			comp.m_spawn_extents,
			comp.m_velocity_min_max_scalar.y,
			comp.m_particle_lifespan_ms, 
			comp.m_particle_spawn_delay_ms, 
			static_cast<int>(comp.m_active),
			0.f,
			comp.m_life_colour_interpolator,
			0.0f, 0.0f,
			comp.m_life_scale_interpolator,
			0.0f, 0.0f,
			comp.m_life_alpha_interpolator,
			0.0f, 0.0f,
			comp.acceleration,
			0.f
			);

		glNamedBufferSubData(m_emitter_ssbo.GetHandle(), index * emitter_struct_size, emitter_struct_size, emitter_data.data());
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	}


	void ParticleSystem::OnEmitterUpdate(ParticleEmitterComponent* p_comp) {
		UpdateEmitterBufferAtIndex(p_comp->m_index);
	}

	void ParticleSystem::OnEmitterVisualTypeChange(ParticleEmitterComponent* p_comp) {
		auto* p_ent = p_comp->GetEntity();

		if (p_comp->m_type == ParticleEmitterComponent::BILLBOARD) {
			p_ent->DeleteComponent<ParticleMeshResources>();
			p_ent->AddComponent<ParticleBillboardResources>()->p_material = AssetManager::GetAsset<Material>(static_cast<uint64_t>(BaseAssetIDs::DEFAULT_MATERIAL));
		}
		else {
			p_ent->DeleteComponent<ParticleBillboardResources>();
			auto* p_res = p_ent->AddComponent<ParticleMeshResources>();
			p_res->p_mesh = AssetManager::GetAsset<MeshAsset>(static_cast<uint64_t>(BaseAssetIDs::CUBE_MESH));
			p_res->materials = { static_cast<size_t>(p_res->p_mesh->GetNbMaterials()), AssetManager::GetAsset<Material>(static_cast<uint64_t>(BaseAssetIDs::DEFAULT_MATERIAL)) };
		}
	}

	void ParticleSystem::OnEmitterUpdate(const Events::ECS_Event<ParticleEmitterComponent>& e_event) {
		auto* p_comp = e_event.p_component;

		if (e_event.sub_event_type & (ParticleEmitterComponent::FULL_UPDATE | ParticleEmitterComponent::NB_PARTICLES_CHANGED | ParticleEmitterComponent::LIFESPAN_CHANGED | ParticleEmitterComponent::SPAWN_DELAY_CHANGED)) {
			OnEmitterDestroy(p_comp, e_event.p_data ? *reinterpret_cast<int*>(e_event.p_data) : 0);
			InitEmitter(p_comp);
		}
		else if (e_event.sub_event_type & ParticleEmitterComponent::VISUAL_TYPE_CHANGED) {
			OnEmitterVisualTypeChange(e_event.p_component);
		}
		else if (e_event.sub_event_type & ParticleEmitterComponent::ACTIVE_STATUS_CHANGED) {
			e_event.p_component->m_last_active_status_change_time = FrameTiming::GetTotalElapsedTime();
			UpdateEmitterBufferAtIndex(p_comp->m_index);
			if (p_comp->IsActive()) {
				InitParticles(p_comp);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			}
		}
		else {
			UpdateEmitterBufferAtIndex(e_event.p_component->m_index);
		}
	}



	void ParticleSystem::OnEmitterDestroy(ParticleEmitterComponent* p_comp, int dif) {
		// Dif != 0 if the component has not had all its particles allocated
		int old_nb_particles = static_cast<int>(p_comp->m_num_particles) - dif;
		ASSERT(old_nb_particles >= 0);

		int num_particles_to_decrement = (total_emitter_particles - (static_cast<int>(p_comp->m_particle_start_index) + old_nb_particles));
		if (num_particles_to_decrement > 0) {
			// Adjust emitter indices to account for deletion (handled in shader)
			m_particle_initializer_cs.Activate(ParticleCSVariants::EMITTER_DELETE_DECREMENT_PARTICLES);
			m_particle_initializer_cs.SetUniform("u_emitter_index", p_comp->m_index);
			m_particle_initializer_cs.SetUniform("u_start_index", p_comp->m_particle_start_index + p_comp->m_num_particles);
			m_particle_initializer_cs.SetUniform("u_num_particles", static_cast<unsigned>(num_particles_to_decrement));
			GL_StateManager::DispatchCompute(static_cast<int>(glm::ceil(static_cast<float>(num_particles_to_decrement) / 32.f)), 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		// Emitter "start indices" (where their particles start in the buffer) need to be modified to account for deletion
		m_particle_initializer_cs.Activate(ParticleCSVariants::EMITTER_DELETE_DECREMENT_EMITTERS);
		m_particle_initializer_cs.SetUniform("u_emitter_index", p_comp->m_index);
		m_particle_initializer_cs.SetUniform("u_num_emitters", static_cast<unsigned>(m_emitter_entities.size()));
		GL_StateManager::DispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		auto it = std::ranges::find(m_emitter_entities, p_comp->GetEnttHandle());

		for (auto i = it + 1; i < m_emitter_entities.end(); i++) {
			auto& em = mp_scene->GetRegistry().get<ParticleEmitterComponent>(*i);
			em.m_particle_start_index -= static_cast<unsigned>(old_nb_particles);
			em.m_index--;
		}

		m_emitter_entities.erase(it);
		total_emitter_particles -= old_nb_particles;

		m_emitter_ssbo.Erase(p_comp->m_index * emitter_struct_size, emitter_struct_size);
		m_particle_ssbo.Erase(p_comp->m_particle_start_index * particle_struct_size, static_cast<unsigned>(old_nb_particles) * particle_struct_size);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

		GL_StateManager::BindSSBO(m_emitter_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_EMITTERS);
		GL_StateManager::BindSSBO(m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
	}

	void ParticleSystem::OnUnload() {
		Events::EventManager::DeregisterListener(m_transform_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_particle_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_particle_buffer_listener.GetRegisterID());

		for (auto& connection : m_connections) {
			connection.release();
		}
	}

	void ParticleSystem::OnUpdate() {
		m_particle_cs.ActivateProgram();
		GL_StateManager::BindSSBO(m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);

		if (total_emitter_particles > 0)
			GL_StateManager::DispatchCompute(static_cast<int>(glm::ceil(static_cast<float>(total_emitter_particles) / 64.f)), 1, 1);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
}
