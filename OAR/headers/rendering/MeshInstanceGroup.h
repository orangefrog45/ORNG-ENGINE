#pragma once
#include "rendering/MeshAsset.h"
#include "components/MeshComponent.h"

class WorldTransform;
class Scene;

namespace ORNG {

	class MeshInstanceGroup {
	public:
		friend class EditorLayer;
		friend class Renderer;
		friend class MeshComponent;
		friend class Scene;
		friend class RenderPasses;
		MeshInstanceGroup(MeshAsset* t_mesh_data, unsigned int shader_id, Scene* scene, unsigned int material_id) :
			m_mesh_asset(t_mesh_data), m_group_shader_id(shader_id), m_scene(scene), m_group_material_id(material_id), m_transform_ssbo_handle(t_mesh_data->m_vao.GenTransformSSBO()) {};
		~MeshInstanceGroup() {
			glDeleteBuffers(1, &m_transform_ssbo_handle);
		}

		void AddMeshPtr(MeshComponent* ptr);

		//Clears all mesh ptrs from group, use for deletion
		void ClearMeshes();

		//Deletes mesh component from instance group, mesh will need to be resorted into different instance group after
		void DeleteMeshPtr(MeshComponent* ptr);

		//sorts mesh into different instance group upon it being changed, meshes will request this
		void ResortMesh(MeshComponent* ptr);

		int FindMeshPtrIndex(const MeshComponent* ptr);

		/* Process deletion/addition of meshes, transform changes  */
		void ProcessUpdates();


		auto GetMeshData() const { return m_mesh_asset; }
		auto GetShaderID() const { return m_group_shader_id; }
		unsigned int GetInstanceCount() const { return m_meshes.size(); }
		unsigned int GetMaterialID() const { return m_group_material_id; }


	private:
		bool m_update_world_mat_buffer_flag = false;
		bool m_sub_update_world_mat_buffer_flag = false;

		/* Activates flag that will cause the instance group owning this mesh to check for mesh transform updates */
		void ActivateFlagSubUpdateWorldMatBuffer() { m_sub_update_world_mat_buffer_flag = true; };

		/* Update entire transform buffer with current mesh transforms */
		void UpdateTransformSSBO(); // needs to be called upon m_meshes growing/shrinking

		std::vector< MeshComponent*> m_meshes;
		Scene* m_scene = nullptr;
		MeshAsset* m_mesh_asset;
		unsigned int m_transform_ssbo_handle = 0;
		unsigned int m_group_shader_id;
		unsigned int m_group_material_id;
	};

}