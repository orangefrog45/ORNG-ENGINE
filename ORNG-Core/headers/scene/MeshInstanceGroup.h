#pragma once
#include "components/MeshComponent.h"
#include "rendering/VAO.h"


namespace ORNG {
	class TransformComponent;
	class Scene;
	class MeshInstancingSystem;
	class MeshAsset;

	struct InstanceData {
		InstanceData(SceneEntity* entity, unsigned i) : p_entity(entity), index(i) {};
		SceneEntity* p_entity = nullptr;
		unsigned index = 0;
	};

	struct InstanceUpdateData {
		InstanceUpdateData(SceneEntity* ent, const glm::mat4& transform) : p_entity(ent), new_transform(transform) {};
		SceneEntity* p_entity = nullptr;
		glm::mat4 new_transform;
	};

	class MeshInstanceGroup {
	public:
		friend class MeshInstancingSystem;
		friend class Scene;
		friend class SceneRenderer;

		// Constructor for mesh component instance groups
		MeshInstanceGroup(MeshAsset* t_mesh_data, MeshInstancingSystem* p_mcm, const std::vector<const Material*>& materials, entt::registry& registry);

		// Constructor for billboard instance groups
		MeshInstanceGroup(MeshAsset* t_mesh_data, MeshInstancingSystem* p_mcm, const Material* p_material, entt::registry& registry) :
			m_mesh_asset(t_mesh_data), m_registry(registry)
		{
			m_materials.push_back(p_material);
			// Setup a transform matrix ssbo for this instance group
			m_transform_ssbo.Init();
			m_tombstone_limit = 250;
		};


		void AddInstance(SceneEntity* ptr);

		//Clears all mesh ptrs from group, use for deletion
		void ClearMeshes();

		//Deletes mesh component from instance group, mesh will need to be resorted into different instance group after
		void RemoveInstance(SceneEntity* ptr);

		/* Process deletion/addition of meshes, transform changes  */
		void ProcessUpdates();

		void FlagInstanceTransformUpdate(SceneEntity* p_instance);

		MeshAsset* GetMeshAsset() const { return m_mesh_asset; }

		unsigned GetInstanceCount() const { return (unsigned)m_instances.size(); }

		// Tombstones still need to be rendered invisibly, their transforms are all set to scale 0 so they only incur a cost at the vertex shader stage
		unsigned GetRenderCount() const { return (unsigned)m_instances.size() + m_tombstone_count; }

		const std::vector<const Material*>& GetMaterialIDs() const { return m_materials; }

	private:
		void ReallocateInstances();

		// Key = entity, Val = transform buffer index
		std::unordered_map<entt::entity, unsigned> m_instances;

		//ID's of materials associated with each submesh of the mesh asset
		std::vector<const Material*> m_materials;
		MeshAsset* m_mesh_asset;

		std::vector<entt::entity> m_instances_to_update;

		std::vector<entt::entity> m_entities_to_instance;

		entt::registry& m_registry;

		unsigned m_tombstone_count = 0;

		// Where the "used" memory in the transform buffer ends, the rest can be overwritten
		unsigned m_used_transform_memory_end_idx = 0;

		unsigned m_tombstone_limit;

		SSBO<float> m_transform_ssbo{ true, 0};

	};
}