#pragma once
#include "components/MeshComponent.h"


namespace ORNG {
	class TransformComponent;
	class Scene;
	class MeshInstancingSystem;
	class MeshAsset;

	class MeshInstanceGroup {
	public:
		friend class MeshInstancingSystem;
		friend class Scene;
		friend class SceneRenderer;
		MeshInstanceGroup(MeshAsset* t_mesh_data, MeshInstancingSystem* p_mcm, const std::vector<const Material*>& materials) :
			m_mesh_asset(t_mesh_data), m_materials(materials)
		{
			// Setup a transform matrix ssbo for this instance group
			m_transform_ssbo_handle = t_mesh_data->m_vao.GenTransformSSBO();
		};

		~MeshInstanceGroup() {
			m_mesh_asset->m_vao.DeleteTransformSSBO(m_transform_ssbo_handle);
		}

		void AddMeshPtr(MeshComponent* ptr);

		//Clears all mesh ptrs from group, use for deletion
		void ClearMeshes();

		//Deletes mesh component from instance group, mesh will need to be resorted into different instance group after
		void DeleteMeshPtr(MeshComponent* ptr);

		int FindMeshPtrIndex(const MeshComponent* ptr);

		/* Process deletion/addition of meshes, transform changes  */
		void ProcessUpdates();


		auto GetMeshData() const { return m_mesh_asset; }
		size_t GetInstanceCount() const { return m_instances.size(); }
		const std::vector<const Material*>& GetMaterialIDs() const { return m_materials; }


		/* Activates flag that will cause the instance group to check for mesh transform updates */
		void ActivateFlagSubUpdateWorldMatBuffer() { m_sub_update_world_mat_buffer_flag = true; };


	private:
		bool m_update_world_mat_buffer_flag = false;
		bool m_sub_update_world_mat_buffer_flag = false;
		/* Update entire transform buffer with current mesh transforms */
		void UpdateTransformSSBO(); // needs to be called upon m_meshes growing/shrinking

		std::vector< MeshComponent*> m_instances;
		//ID's of materials associated with each submesh of the mesh asset
		std::vector<const Material*> m_materials;

		MeshAsset* m_mesh_asset;
		unsigned int m_transform_ssbo_handle = 0;
	};

}