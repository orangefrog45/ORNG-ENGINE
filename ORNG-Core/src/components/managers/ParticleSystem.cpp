#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "core/GLStateManager.h"
#include "rendering/Renderer.h"
#include "scene/SceneEntity.h"
#include "assets/AssetManager.h"
#include "util/TimeStep.h"
#include "components/ParticleBufferComponent.h"
#include "util/Timers.h"




namespace ORNG {

	constexpr unsigned particle_struct_size = sizeof(float) * 4 + sizeof(glm::vec4) * 4;
	constexpr unsigned emitter_struct_size = sizeof(float) * 36 + InterpolatorV3::GPU_STRUCT_SIZE_BYTES * 2 + sizeof(float) * 6 + InterpolatorV1::GPU_STRUCT_SIZE_BYTES + sizeof(float) * 3;
	constexpr unsigned particle_transform_size = sizeof(float) * 12;

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
		if (!mp_particle_cs) {
			mp_particle_cs = &Renderer::GetShaderLibrary().CreateShader("particle update");
			mp_particle_cs->AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/ParticleCS.glsl");
			mp_particle_cs->Init();


			mp_particle_initializer_cs = &Renderer::GetShaderLibrary().CreateShaderVariants("particle initializer");
			mp_particle_initializer_cs->SetPath(GL_COMPUTE_SHADER, "res/core-res/shaders/ParticleInitializerCS.glsl");
			mp_particle_initializer_cs->AddVariant(ParticleCSVariants::DEFAULT, {}, { "u_start_index", "u_emitter_index" });
			mp_particle_initializer_cs->AddVariant(ParticleCSVariants::EMITTER_DELETE_DECREMENT_EMITTERS, { "EMITTER_DELETE", "EMITTER_DELETE_DECREMENT_EMITTERS" }, { "u_emitter_index", "u_num_emitters" });
			mp_particle_initializer_cs->AddVariant(ParticleCSVariants::EMITTER_DELETE_DECREMENT_PARTICLES, { "EMITTER_DELETE", "EMITTER_DELETE_DECREMENT_PARTICLES" }, { "u_start_index", "u_num_particles" });
			mp_particle_initializer_cs->AddVariant(ParticleCSVariants::INITIALIZE_AS_DEAD, { "INITIALIZE_AS_DEAD" }, { "u_start_index" });

			mp_append_buffer_transfer_cs = &Renderer::GetShaderLibrary().CreateShader("particle append buffer transfer");
			mp_append_buffer_transfer_cs->AddStage(GL_COMPUTE_SHADER, "res/core-res/shaders/ParticleAppendTransferCS.glsl");
			mp_append_buffer_transfer_cs->Init();
			mp_append_buffer_transfer_cs->AddUniform("u_buffer_index");
		}

	
	};

	void ParticleSystem::OnLoad() {
		if (!m_particle_ssbo.IsInitialized()) {
			m_particle_ssbo.Init();
			m_emitter_ssbo.Init();
			m_particle_append_ssbo.Init();
			m_particle_append_ssbo.Resize(sizeof(uint32_t) + particle_struct_size * 100'000);
			p_num_appended = static_cast<unsigned*>(glMapNamedBufferRange(m_particle_append_ssbo.GetHandle(), 0, sizeof(unsigned), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT));
		}



		m_particle_append_ssbo.draw_type = GL_DYNAMIC_DRAW;
		m_particle_ssbo.draw_type = GL_DYNAMIC_DRAW;
		m_emitter_ssbo.draw_type = GL_DYNAMIC_DRAW;

		GL_StateManager::BindSSBO(m_emitter_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_EMITTERS);
		GL_StateManager::BindSSBO(m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
		GL_StateManager::BindSSBO(m_particle_append_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_APPEND);


		m_particle_listener.scene_id = GetSceneUUID();
		m_particle_listener.OnEvent = [this](const Events::ECS_Event<ParticleEmitterComponent>& e_event) {
			switch (e_event.event_type) {
				using enum Events::ECS_EventType;
			case COMP_ADDED:
				InitEmitter(e_event.affected_components[0]);
				break;
			case COMP_UPDATED:
				OnEmitterUpdate(e_event);
				break;
			case COMP_DELETED:
				OnEmitterDestroy(e_event.affected_components[0]);
				break;
			}
			};

		m_particle_buffer_listener.scene_id = GetSceneUUID();
		m_particle_buffer_listener.OnEvent = [this](const Events::ECS_Event<ParticleBufferComponent>& e_event) {
			switch (e_event.event_type) {
				using enum Events::ECS_EventType;
			case COMP_ADDED:
				InitBuffer(e_event.affected_components[0]);
				break;
			case COMP_UPDATED:
				OnBufferUpdate(e_event.affected_components[0]);
				break;
			}
			};

		m_transform_listener.scene_id = GetSceneUUID();
		m_transform_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& e_event) {
			if (e_event.event_type == Events::ECS_EventType::COMP_UPDATED) {
				if (auto* p_emitter = e_event.affected_components[0]->GetEntity()->GetComponent<ParticleEmitterComponent>())
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





	void ParticleSystem::InitEmitter(ParticleEmitterComponent* p_comp) {
		auto* p_ent = p_comp->GetEntity();
		auto* p_bb_res = p_ent->GetComponent<ParticleBillboardResources>();
		auto* p_mesh_res = p_ent->GetComponent<ParticleMeshResources>();


		if (p_comp->m_type == ParticleEmitterComponent::BILLBOARD && !p_bb_res) {
			p_bb_res = p_ent->AddComponent<ParticleBillboardResources>();
			p_bb_res->p_material = AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID);
		}
		else if (p_comp->m_type == ParticleEmitterComponent::MESH && !p_mesh_res) {
			p_mesh_res = p_ent->AddComponent<ParticleMeshResources>();
			p_mesh_res->materials = { AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID) };
			p_mesh_res->p_mesh = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID);
		}

		if (!m_emitter_entities.empty()) {
			auto& comp = mp_scene->GetRegistry().get<ParticleEmitterComponent>(m_emitter_entities[m_emitter_entities.size() - 1]);
			p_comp->m_particle_start_index = comp.m_particle_start_index + comp.m_num_particles;
		}
		else {
			p_comp->m_particle_start_index = 0;
		}


		m_emitter_entities.push_back(p_comp->GetEnttHandle());
		p_comp->m_index = m_emitter_entities.size() - 1;



		unsigned prev_total = total_emitter_particles;
		total_emitter_particles += p_comp->m_num_particles;

		if (m_particle_ssbo.GetGPU_BufferSize() <= total_emitter_particles * particle_struct_size ) {

			unsigned new_allocated_particles = (unsigned)glm::ceil(glm::max(total_emitter_particles * 1.5f, 10'000.f));
			m_particle_ssbo.Resize(new_allocated_particles * particle_struct_size);

			mp_particle_initializer_cs->Activate(ParticleCSVariants::INITIALIZE_AS_DEAD);
			mp_particle_initializer_cs->SetUniform("u_start_index", p_comp->m_particle_start_index + p_comp->m_num_particles);
			GL_StateManager::DispatchCompute((int)glm::ceil((new_allocated_particles - (p_comp->m_particle_start_index + p_comp->m_num_particles)) / 32.f), 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		}

		if (m_emitter_ssbo.GetGPU_BufferSize() < m_emitter_entities.size() * emitter_struct_size)
			m_emitter_ssbo.Resize(glm::max((int)glm::ceil(m_emitter_entities.size() * 1.5f), 10) * emitter_struct_size);


		UpdateEmitterBufferAtIndex(p_comp->m_index);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


		// Initialize new particles in shader
		GL_StateManager::BindSSBO(m_emitter_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_EMITTERS);
		GL_StateManager::BindSSBO(m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);

		mp_particle_initializer_cs->Activate(ParticleCSVariants::DEFAULT);
		mp_particle_initializer_cs->SetUniform("u_start_index", p_comp->m_particle_start_index);
		mp_particle_initializer_cs->SetUniform<unsigned>("u_emitter_index", m_emitter_entities.size() - 1);

		// Initialize new particles
		GL_StateManager::DispatchCompute(glm::ceil((float)p_comp->m_num_particles / 32.f), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);		

	}

	void ParticleSystem::UpdateEmitterBufferAtIndex(unsigned index) {
		auto& comp = mp_scene->GetRegistry().get<ParticleEmitterComponent>(m_emitter_entities[index]);
		auto* p_transform = comp.GetEntity()->GetComponent<TransformComponent>();
		auto [pos, scale, rot] = p_transform->GetAbsoluteTransforms();
		glm::vec3 forward = p_transform->forward;

		std::array<std::byte, emitter_struct_size> emitter_data;
		std::byte* p_byte = &emitter_data[0];

		ConvertToBytes(p_byte,
			pos, 0.0f,
			comp.m_particle_start_index, 
			comp.m_num_particles, 
			comp.m_spread * 2.0f * glm::pi<float>(),
			comp.m_velocity_min_max_scalar.x,
			glm::mat4(ExtraMath::Init3DRotateTransform(rot.x, rot.y, rot.z)),
			comp.m_spawn_extents,
			comp.m_velocity_min_max_scalar.y,
			comp.m_particle_lifespan_ms, 
			comp.m_particle_spawn_delay_ms, 
			static_cast<int>(comp.m_active), 0.0f,
			comp.m_life_colour_interpolator,
			0.0f, 0.0f,
			comp.m_life_scale_interpolator,
			0.0f, 0.0f,
			comp.m_life_alpha_interpolator,
			0.0f, 0.0f,
			comp.acceleration,
			0.f
			);

		glNamedBufferSubData(m_emitter_ssbo.GetHandle(), index * emitter_struct_size, emitter_struct_size, &emitter_data[0]);
		//std::memcpy(p_emitter_gpu_buffer + index * emitter_struct_size, &emitter_data[0], emitter_struct_size);
	}


	void ParticleSystem::OnEmitterUpdate(ParticleEmitterComponent* p_comp) {
		UpdateEmitterBufferAtIndex(p_comp->m_index);
	}

	void ParticleSystem::OnEmitterVisualTypeChange(ParticleEmitterComponent* p_comp) {
		auto* p_ent = p_comp->GetEntity();

		if (p_comp->m_type == ParticleEmitterComponent::BILLBOARD) {
			p_ent->DeleteComponent<ParticleMeshResources>();
			p_ent->AddComponent<ParticleBillboardResources>()->p_material = AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID);
		}
		else {
			p_ent->DeleteComponent<ParticleBillboardResources>();
			auto* p_res = p_ent->AddComponent<ParticleMeshResources>();
			p_res->p_mesh = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID);
			p_res->materials = { AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID) };
		}
	}

	void ParticleSystem::OnEmitterUpdate(const Events::ECS_Event<ParticleEmitterComponent>& e_event) {
		auto* p_comp = e_event.affected_components[0];

		if (e_event.sub_event_type & (ParticleEmitterComponent::FULL_UPDATE | ParticleEmitterComponent::NB_PARTICLES_CHANGED | ParticleEmitterComponent::LIFESPAN_CHANGED | ParticleEmitterComponent::SPAWN_DELAY_CHANGED)) {
			OnEmitterDestroy(p_comp, std::any_cast<int>(e_event.data_payload));
			InitEmitter(p_comp);
		}
		else if (e_event.sub_event_type & ParticleEmitterComponent::VISUAL_TYPE_CHANGED) {
			OnEmitterVisualTypeChange(e_event.affected_components[0]);
		}
		else {
			UpdateEmitterBufferAtIndex(e_event.affected_components[0]->m_index);
		}
	}



	void ParticleSystem::OnEmitterDestroy(ParticleEmitterComponent* p_comp, unsigned dif) {
		// Dif != 0 if the component has not had all its particles allocated
		int old_nb_particles = p_comp->m_num_particles - dif;
		ASSERT(old_nb_particles >= 0);


		int num_particles_to_decrement = (total_emitter_particles - (p_comp->m_particle_start_index + old_nb_particles));
		if (num_particles_to_decrement > 0) {
			// Adjust emitter indices to account for deletion (handled in shader)
			mp_particle_initializer_cs->Activate(ParticleCSVariants::EMITTER_DELETE_DECREMENT_PARTICLES);
			mp_particle_initializer_cs->SetUniform("u_emitter_index", p_comp->m_index);
			mp_particle_initializer_cs->SetUniform("u_start_index", p_comp->m_particle_start_index + p_comp->m_num_particles);
			mp_particle_initializer_cs->SetUniform<unsigned>("u_num_particles", num_particles_to_decrement);
			GL_StateManager::DispatchCompute((int)glm::ceil(num_particles_to_decrement / 32.f), 1, 1);
		}

		// Emitter "start indices" (where their particles start in the buffer) need to be modified to account for deletion
		mp_particle_initializer_cs->Activate(ParticleCSVariants::EMITTER_DELETE_DECREMENT_EMITTERS);
		mp_particle_initializer_cs->SetUniform("u_emitter_index", p_comp->m_index);
		mp_particle_initializer_cs->SetUniform<unsigned>("u_num_emitters", m_emitter_entities.size());
		GL_StateManager::DispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		auto it = std::ranges::find(m_emitter_entities, p_comp->GetEnttHandle());

		for (auto i = it + 1; i < m_emitter_entities.end(); i++) {
			auto& em = mp_scene->GetRegistry().get<ParticleEmitterComponent>(*i);
			em.m_particle_start_index -= old_nb_particles;
			em.m_index--;
		}

		m_emitter_entities.erase(it);
		total_emitter_particles -= old_nb_particles;

		m_emitter_ssbo.Erase(p_comp->m_index * emitter_struct_size, emitter_struct_size);
		m_particle_ssbo.Erase(p_comp->m_particle_start_index * particle_struct_size, old_nb_particles * particle_struct_size);

		GL_StateManager::BindSSBO(m_emitter_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_EMITTERS);
		GL_StateManager::BindSSBO(m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
	}

	void ParticleSystem::OnUnload() {
		Events::EventManager::DeregisterListener(m_transform_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_particle_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_particle_buffer_listener.GetRegisterID());
	}

	void ParticleSystem::InitBuffer(ParticleBufferComponent* p_comp) {

		p_comp->m_particle_ssbo.Init();
		p_comp->m_particle_ssbo.Resize(p_comp->m_min_allocated_particles * particle_struct_size);
		

		glNamedBufferSubData(p_comp->m_particle_ssbo.GetHandle(), 0, sizeof(unsigned), &p_comp->m_buffer_id);
	}

	void ParticleSystem::OnBufferUpdate(ParticleBufferComponent* p_comp) {
		p_comp->m_particle_ssbo.Resize(p_comp->m_min_allocated_particles * particle_struct_size);

		glNamedBufferSubData(p_comp->m_particle_ssbo.GetHandle(), 0, sizeof(unsigned), &p_comp->m_buffer_id);
	}



	void ParticleSystem::UpdateAppendBuffer() {
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		if (*p_num_appended == 0)
			return;


		for (auto [entity, buf_comp] : mp_scene->GetRegistry().view<ParticleBufferComponent>().each()) {
			GL_StateManager::BindSSBO(buf_comp.m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);

			mp_append_buffer_transfer_cs->ActivateProgram();
			mp_append_buffer_transfer_cs->SetUniform<unsigned>("u_buffer_index", buf_comp.GetBufferID());
			GL_StateManager::DispatchCompute((int)glm::ceil(*p_num_appended / 32.f), 1, 1);
		}

		unsigned reset = 0;
		memcpy(p_num_appended, &reset, sizeof(unsigned));

	}

	void ParticleSystem::OnUpdate() {
		mp_particle_cs->ActivateProgram();
		GL_StateManager::BindSSBO(m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);

		if (total_emitter_particles > 0)
			GL_StateManager::DispatchCompute(glm::ceil(total_emitter_particles / 32.f), 1, 1);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		UpdateAppendBuffer();
	}
}