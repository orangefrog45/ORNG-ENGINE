#pragma once
#include "components/ComponentAPI.h"
#include "scene/MeshInstanceGroup.h"

namespace physx {
	class PxScene;
	class PxBroadPhase;
	class PxAABBManager;
	class PxCpuDispatcher;
	class PxMaterial;
	class PxTriangleMesh;
}

namespace ORNG {
	class Scene;

	class ComponentManager {
		virtual void OnUpdate() {};
		virtual void OnUnload() {};
	};


	class CameraSystem : public ComponentManager {
	public:

		void OnUnload() final;
		void OnLoad() {};
		CameraComponent* AddComponent(SceneEntity* p_entity);
		CameraComponent* GetComponent(uint64_t entity_id);
		void DeleteComponent(SceneEntity* p_entity);

		void OnUpdate() {
			if (p_active_camera)
				p_active_camera->Update();

		}
		void SetActiveCamera(CameraComponent* p_cam) {
			if (p_active_camera)
				p_active_camera->is_active = false;

			p_active_camera = p_cam;
			p_cam->is_active = true;
		}

		CameraComponent* p_active_camera = nullptr;
	private:
		std::vector<CameraComponent*> m_camera_components;
	};

	class PhysicsSystem : public ComponentManager {
	public:
		friend class EditorLayer;

		void OnUpdate(float ts);
		void OnUnload() final;
		void OnLoad();
		physx::PxTriangleMesh* GetOrCreateTriangleMesh(const MeshAsset* p_mesh_data);
		PhysicsComponent* AddComponent(SceneEntity* p_entity, PhysicsComponent::RigidBodyType type = PhysicsComponent::STATIC);
		PhysicsComponent* GetComponent(uint64_t entity_id);
		void DeleteComponent(SceneEntity* p_entity);

	private:

		bool m_physics_paused = true;

		physx::PxBroadPhase* mp_broadphase = nullptr;
		physx::PxAABBManager* mp_aabb_manager = nullptr;
		physx::PxScene* mp_scene = nullptr;
		std::vector<PhysicsComponent*> m_physics_components;

		std::vector<physx::PxMaterial*> m_physics_materials;

		std::unordered_map<const MeshAsset*, physx::PxTriangleMesh*> m_triangle_meshes;
		float m_step_size = 1.f / 60.f;
		float m_accumulator = 0.f;


	};

	class TransformComponentManager : public ComponentManager {
	public:
		void OnUpdate() final { /* Not needed currently for transforms */ };
		void OnUnload() final;

		TransformComponent* AddComponent(SceneEntity* p_entity);
		TransformComponent* GetComponent(uint64_t entity_id);

	private:
		std::vector<TransformComponent*> m_transform_components;
	};


	class SpotlightComponentManager : public ComponentManager {
	public:
		void OnLoad();
		void OnUpdate() final;
		void OnUnload() final;
		SpotLightComponent* AddComponent(SceneEntity* p_entity);
		SpotLightComponent* GetComponent(uint64_t entity_id);
		const auto& GetComponents() const { return m_spotlight_components; }
		void DeleteComponent(SceneEntity* p_entity);
	private:
		std::vector<SpotLightComponent*> m_spotlight_components;
		GLuint m_spotlight_ssbo_handle;
	};



	class PointlightComponentManager : public ComponentManager {
	public:
		void OnLoad();
		void OnUpdate() final;
		void OnUnload() final;
		PointLightComponent* AddComponent(SceneEntity* p_entity);
		PointLightComponent* GetComponent(uint64_t entity_id);
		void DeleteComponent(SceneEntity* p_entity);

		const auto& GetComponents() { return m_pointlight_components; }
	private:
		std::vector<PointLightComponent*> m_pointlight_components;
		GLuint m_pointlight_ssbo_handle;
	};


	class MeshComponentManager : public ComponentManager {
	public:
		void SortMeshIntoInstanceGroup(MeshComponent* comp, MeshAsset* asset);
		MeshComponent* AddComponent(SceneEntity* p_entity, MeshAsset* asset);
		MeshComponent* GetComponent(uint64_t entity_id);
		void DeleteComponent(SceneEntity* p_entity);
		void OnLoad();
		void OnUnload() final;
		void OnUpdate() final;
		void OnMeshAssetDeletion(MeshAsset* p_asset);
		void OnMaterialDeletion(Material* p_material, Material* p_replacement_material);

		const auto& GetInstanceGroups() { return m_instance_groups; }
		const auto& GetMeshComponents() { return m_mesh_components; }
	private:
		std::vector<MeshInstanceGroup*> m_instance_groups;
		std::vector<MeshComponent*> m_mesh_components;
	};
}