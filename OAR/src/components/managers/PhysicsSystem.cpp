#include "pch/pch.h"
#include "components/ComponentManagers.h"
#include "physics/Physics.h"
#include "scene/SceneEntity.h"
#include "glm/glm/gtc/quaternion.hpp"


namespace ORNG {

	void PhysicsSystem::OnLoad() {

		PxBroadPhaseDesc bpDesc(PxBroadPhaseType::eABP);

		mp_broadphase = PxCreateBroadPhase(bpDesc);
		mp_aabb_manager = PxCreateAABBManager(*mp_broadphase);

		PxSceneDesc scene_desc{ Physics::GetToleranceScale() };
		scene_desc.filterShader = PxDefaultSimulationFilterShader;
		scene_desc.gravity = PxVec3(0, -9.81, 0);
		scene_desc.cpuDispatcher = Physics::GetCPUDispatcher();
		scene_desc.cudaContextManager = Physics::GetCudaContextManager();
		scene_desc.flags |= PxSceneFlag::eENABLE_GPU_DYNAMICS;
		scene_desc.flags |= PxSceneFlag::eENABLE_PCM;
		scene_desc.broadPhaseType = PxBroadPhaseType::eGPU;
		PxCudaContextManagerDesc desc;


		mp_scene = Physics::GetPhysics()->createScene(scene_desc);

		mp_scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1.0f);
		mp_scene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1.0f);
		m_physics_materials.push_back(Physics::GetPhysics()->createMaterial(0.25f, 0.1f, 0.1f));
		m_physics_materials[0]->acquireReference();
	}




	void PhysicsSystem::DeleteComponent(SceneEntity* p_entity) {
		auto it = std::find_if(m_physics_components.begin(), m_physics_components.end(), [=](auto* p_comp) {return p_comp->GetEntityHandle() == p_entity->GetID(); });

		if (it == m_physics_components.end())
			return;

		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		p_transform->update_callbacks.erase(TransformComponent::CallbackType::PHYSICS);

		delete* it;
		m_physics_components.erase(it);
	}




	void PhysicsSystem::OnUnload() {
		for (auto* p_comp : m_physics_components) {
			auto* p_transform = p_comp->mp_transform;
			p_transform->update_callbacks.erase(TransformComponent::CallbackType::PHYSICS);
			delete p_comp;
		}

		m_physics_components.clear();

		for (auto* p_material : m_physics_materials) {
			p_material->release();
		}
		m_physics_materials.clear();

		mp_scene->release();
		mp_aabb_manager->release();
		PX_RELEASE(mp_broadphase);
	}



	PxTriangleMesh* PhysicsSystem::GetOrCreateTriangleMesh(const MeshAsset* p_mesh_asset) {
		if (m_triangle_meshes.contains(p_mesh_asset))
			return m_triangle_meshes[p_mesh_asset];



		const VAO& vao = p_mesh_asset->GetVAO();
		PxCookingParams params(Physics::GetToleranceScale());
		params.buildGPUData = true;
		// disable mesh cleaning - perform mesh validation on development configurations
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
		// disable edge precompute, edges are set for each triangle, slows contact generation
		params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
		// lower hierarchy for internal mesh
		//params.meshCookingHint = PxMeshCookingHint::eCOOKING_PERFORMANCE;

		PxSDFDesc desc = PxSDFDesc();

		desc.subgridSize = 4;
		desc.spacing = 1;
		PxTriangleMeshDesc meshDesc;
		meshDesc.points.count = vao.vertex_data.positions.size();
		meshDesc.points.stride = sizeof(glm::vec3);
		meshDesc.points.data = (void*)&vao.vertex_data.positions[0];

		meshDesc.triangles.count = vao.vertex_data.indices.size() / 3;
		meshDesc.triangles.stride = 3 * sizeof(unsigned int);
		meshDesc.triangles.data = (void*)&vao.vertex_data.indices[0];
		meshDesc.sdfDesc = &desc;
#ifdef _DEBUG
		// mesh should be validated before cooked without the mesh cleaning
		bool res = PxValidateTriangleMesh(params, meshDesc);
		PX_ASSERT(res);
#endif

		PxTriangleMesh* aTriangleMesh = PxCreateTriangleMesh(params, meshDesc, Physics::GetPhysics()->getPhysicsInsertionCallback());
		m_triangle_meshes[p_mesh_asset] = aTriangleMesh;
		return aTriangleMesh;
	}



	void PhysicsSystem::OnUpdate(float ts) {

		if (m_physics_paused)
			return;

		m_accumulator += ts;

		if (m_accumulator < m_step_size)
			return;

		m_accumulator -= m_step_size;
		mp_scene->simulate(m_step_size);
		mp_scene->fetchResults(true);

		for (auto* p_comp : m_physics_components) {
			if (p_comp->rigid_body_type == PhysicsComponent::STATIC || static_cast<PxRigidDynamic*>(p_comp->p_rigid_actor)->isSleeping())
				continue;


			PxTransform transform = p_comp->p_rigid_actor->getGlobalPose();
			PxVec3 phys_pos = transform.p;
			PxQuatT phys_rot = transform.q;

			// Deltas used to get around transform inheritance, if set to absolute transform then transforms would be inherited twice
			glm::quat phys_quat = glm::quat(phys_rot.w, phys_rot.x, phys_rot.y, phys_rot.z);
			glm::vec3 delta_rotation = glm::degrees(glm::eulerAngles(phys_quat)) - p_comp->old_orientation;

			p_comp->mp_transform->SetAbsolutePosition(glm::vec3(phys_pos.x, phys_pos.y, phys_pos.z));
			p_comp->mp_transform->SetOrientation(p_comp->mp_transform->GetRotation() + delta_rotation);
		}
	}





	PhysicsComponent* PhysicsSystem::GetComponent(uint64_t entity_id) {
		auto it = std::find_if(m_physics_components.begin(), m_physics_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
		return it == m_physics_components.end() ? nullptr : *it;
	}




	PhysicsComponent* PhysicsSystem::AddComponent(SceneEntity* p_entity, PhysicsComponent::RigidBodyType type) {

		if (auto* p_found_comp = GetComponent(p_entity->GetID())) {
			OAR_CORE_WARN("Physics component not added to component '{0}', already has one", p_entity->name);
			return p_found_comp;
		}

		auto* p_meshc = p_entity->GetComponent<MeshComponent>();
		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		auto* p_comp = new PhysicsComponent(p_entity, p_transform, this);


		m_physics_components.push_back(p_comp);
		glm::vec3 extents = p_transform->GetScale() * (p_meshc ? p_meshc->GetMeshData()->GetAABB().max : glm::vec3(1));
		p_comp->Init(mp_scene, extents, type, m_physics_materials[0]);

		// Give transform update callback to update the physics actor transform whenever the transform componnent of this entity updates
		p_transform->update_callbacks[TransformComponent::CallbackType::PHYSICS] = ([p_comp, p_transform](TransformComponent::UpdateType t_update_type) {

			auto transforms = p_transform->GetAbsoluteTransforms();
			glm::vec3 abs_pos = transforms[0];
			glm::quat quat{glm::radians(transforms[2])};


			PxVec3 px_pos{ abs_pos.x, abs_pos.y, abs_pos.z };
			PxQuat px_quat{ quat.x, quat.y, quat.z, quat.w };
			PxTransform transform{ px_pos, px_quat };

			p_comp->p_rigid_actor->setGlobalPose(transform);

			p_comp->old_orientation = glm::degrees(glm::eulerAngles(quat));
			p_comp->old_pos = abs_pos;

			if (t_update_type == TransformComponent::UpdateType::SCALE || t_update_type == TransformComponent::UpdateType::ALL) { // Whole shape needs to be rebuilt
				p_comp->UpdateGeometry(p_comp->geometry_type);
				return;
			}

			});


		return p_comp;
	}

}