#pragma once
#include "components/MeshComponent.h"


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
		MeshInstanceGroup(MeshAsset* t_mesh_data, MeshInstancingSystem* p_mcm, const std::vector<const Material*>& materials) :
			m_mesh_asset(t_mesh_data), m_materials(materials)
		{
			// Setup a transform matrix ssbo for this instance group
			m_transform_ssbo.Init();
		};

		// Constructor for billboard instance groups
		MeshInstanceGroup(MeshAsset* t_mesh_data, MeshInstancingSystem* p_mcm, const Material* p_material) :
			m_mesh_asset(t_mesh_data)
		{
			m_materials.push_back(p_material);
			// Setup a transform matrix ssbo for this instance group
			m_transform_ssbo.Init();
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

		unsigned GetInstanceCount() const { return m_instances.size(); }

		const std::vector<const Material*>& GetMaterialIDs() const { return m_materials; }


	private:
		std::map<SceneEntity*, unsigned> m_instances;

		//ID's of materials associated with each submesh of the mesh asset
		std::vector<const Material*> m_materials;
		MeshAsset* m_mesh_asset;


		std::vector<SceneEntity*> m_instances_to_update;

		SSBO<float> m_transform_ssbo;
	};
}