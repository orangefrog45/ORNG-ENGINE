#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "core/GLStateManager.h"
#include "rendering/Renderer.h"
#include "scene/SceneEntity.h"
#include "assets/AssetManager.h"

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
				OnEmitterUpdate(e_event);
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
	constexpr unsigned emitter_struct_size = sizeof(float) * 36 + InterpolatorV3::GPU_STRUCT_SIZE_BYTES * 3 + sizeof(float) * 9 + InterpolatorV1::GPU_STRUCT_SIZE_BYTES + sizeof(float) * 3;
	constexpr unsigned particle_transform_size = sizeof(float) * 12;

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
		m_transform_ssbo.Resize(total_particles * particle_transform_size);
		std::vector<float> rnd_vec;
		rnd_vec.resize(p_comp->m_num_particles * 12);

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dis(0, 2.f * glm::pi<float>());
		std::uniform_real_distribution<float> dis2(-1.0, 1.0);

		// Put random seed in transform, done here as the GPU RNG's aren't well-distributed enough
		for (unsigned i = 0; i < p_comp->m_num_particles * 12; i) {
			rnd_vec[i++] = dis(gen);
			rnd_vec[i++] = dis(gen);
			rnd_vec[i++] = dis(gen);
			rnd_vec[i++] = dis(gen);

			glm::quat q = glm::normalize(glm::quat(dis2(gen), dis2(gen), dis2(gen), dis2(gen)));
			rnd_vec[i++] = q.x;
			rnd_vec[i++] = q.y;
			rnd_vec[i++] = q.z;
			rnd_vec[i++] = q.w;

			rnd_vec[i++] = 1.0;
			rnd_vec[i++] = 1.0;
			rnd_vec[i++] = 1.0;
			rnd_vec[i++] = 1.0;
		}

		glNamedBufferSubData(m_transform_ssbo.GetHandle(), prev_total * particle_transform_size, p_comp->m_num_particles * particle_transform_size, rnd_vec.data());
		m_emitter_ssbo.Resize(m_emitter_entities.size() * emitter_struct_size);
		m_particle_ssbo.Resize(total_particles * particle_struct_size);

		// Initialize emitter
		UpdateEmitterBufferAtIndex(p_comp->m_index);


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
		auto* p_transform = comp.GetEntity()->GetComponent<TransformComponent>();
		auto abs_transforms = p_transform->GetAbsoluteTransforms();
		glm::vec3 pos = abs_transforms[0];
		glm::vec3 rot = abs_transforms[2];
		glm::vec3 forward = p_transform->forward;

		std::array<std::byte, emitter_struct_size> emitter_data;
		std::byte* p_byte = &emitter_data[0];


		ConvertToBytes(pos.x, p_byte);
		ConvertToBytes(pos.y, p_byte);
		ConvertToBytes(pos.z, p_byte);
		ConvertToBytes(0.f, p_byte); // padding
		ConvertToBytes(comp.m_particle_start_index, p_byte);
		ConvertToBytes(comp.m_num_particles, p_byte);
		ConvertToBytes(comp.m_spread * 2.f * glm::pi<float>(), p_byte);
		ConvertToBytes(comp.m_velocity_min_max_scalar.x, p_byte);
		PushMatrixIntoArrayBytes(ExtraMath::Init3DRotateTransform(rot.x, rot.y, rot.z), p_byte);
		ConvertToBytes(comp.m_spawn_extents.x, p_byte);
		ConvertToBytes(comp.m_spawn_extents.y, p_byte);
		ConvertToBytes(comp.m_spawn_extents.z, p_byte);
		ConvertToBytes(comp.m_velocity_min_max_scalar.y, p_byte);
		ConvertToBytes(comp.m_particle_lifespan_ms, p_byte);
		ConvertToBytes(comp.m_particle_spawn_delay_ms, p_byte);
		ConvertToBytes((int)comp.m_active, p_byte);
		ConvertToBytes(0.0f, p_byte); // padding
		comp.m_life_colour_interpolator.ConvertSelfToBytes(p_byte);
		ConvertToBytes(0.0f, p_byte); // padding
		ConvertToBytes(0.0f, p_byte); // padding
		ConvertToBytes(0.0f, p_byte); // padding
		comp.m_life_scale_interpolator.ConvertSelfToBytes(p_byte);
		ConvertToBytes(0.0f, p_byte); // padding
		ConvertToBytes(0.0f, p_byte); // padding
		ConvertToBytes(0.0f, p_byte); // padding
		comp.m_life_alpha_interpolator.ConvertSelfToBytes(p_byte);
		ConvertToBytes(0.0f, p_byte); // padding
		ConvertToBytes(0.0f, p_byte); // padding
		ConvertToBytes(0.0f, p_byte); // padding
		comp.m_velocity_life_interpolator.ConvertSelfToBytes(p_byte);
		ConvertToBytes(0.0f, p_byte); // padding
		ConvertToBytes(0.0f, p_byte); // padding
		ConvertToBytes(0.0f, p_byte); // padding
		ConvertToBytes(comp.acceleration.x, p_byte); 
		ConvertToBytes(comp.acceleration.y, p_byte); 
		ConvertToBytes(comp.acceleration.z, p_byte); 
		ConvertToBytes(0.0f, p_byte); // padding


		glNamedBufferSubData(m_emitter_ssbo.GetHandle(), index * emitter_struct_size, emitter_struct_size, emitter_data.data());
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

		if (e_event.sub_event_type == ParticleEmitterComponent::NB_PARTICLES_CHANGED || e_event.sub_event_type == ParticleEmitterComponent::LIFESPAN_CHANGED || e_event.sub_event_type == ParticleEmitterComponent::SPAWN_DELAY_CHANGED) {
			OnEmitterDestroy(p_comp, std::any_cast<int>(e_event.data_payload));
			InitEmitter(p_comp);
		}
		else if (e_event.sub_event_type == ParticleEmitterComponent::VISUAL_TYPE_CHANGED) {
			OnEmitterVisualTypeChange(e_event.affected_components[0]);
		}
		else {
			UpdateEmitterBufferAtIndex(e_event.affected_components[0]->m_index);
		}
	}



	void ParticleSystem::OnEmitterDestroy(ParticleEmitterComponent* p_comp, unsigned dif) {
		unsigned old_nb_particles = p_comp->m_num_particles - dif;
		// Adjust emitter indices to account for deletion (handled in shader)
		mp_particle_initializer_cs->Activate(ParticleCSVariants::EMITTER_DELETE);
		mp_particle_initializer_cs->SetUniform("u_emitter_index", p_comp->m_index);
		// TODO: Parallelize
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		auto it = std::ranges::find(m_emitter_entities, p_comp->GetEnttHandle());

		for (auto i = it + 1; i < m_emitter_entities.end(); i++) {
			auto& em = mp_registry->get<ParticleEmitterComponent>(*i);
			em.m_particle_start_index -= old_nb_particles;
			em.m_index--;
		}

		m_emitter_entities.erase(it);
		total_particles -= old_nb_particles;

		m_transform_ssbo.Erase(p_comp->m_particle_start_index * particle_transform_size, old_nb_particles * particle_transform_size);
		m_emitter_ssbo.Erase(p_comp->m_index * emitter_struct_size, emitter_struct_size);
		m_particle_ssbo.Erase(p_comp->m_particle_start_index * particle_struct_size, old_nb_particles * particle_struct_size);

		GL_StateManager::BindSSBO(m_emitter_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_EMITTERS);
		GL_StateManager::BindSSBO(m_transform_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_TRANSFORMS);
		GL_StateManager::BindSSBO(m_particle_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLES);
	}

	void ParticleSystem::OnUnload() {
		Events::EventManager::DeregisterListener(m_transform_listener.GetRegisterID());
		Events::EventManager::DeregisterListener(m_particle_listener.GetRegisterID());
	}

	void ParticleSystem::OnUpdate() {
		GL_StateManager::BindSSBO(m_transform_ssbo.GetHandle(), GL_StateManager::SSBO_BindingPoints::PARTICLE_TRANSFORMS);
		mp_particle_cs->ActivateProgram();
		// Each thread processes 4 particles
		glDispatchCompute(glm::ceil((total_particles / 32.f) / 4.f), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
}