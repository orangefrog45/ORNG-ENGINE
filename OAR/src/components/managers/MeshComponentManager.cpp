#include "pch/pch.h"
#include "components/ComponentManagers.h"
#include "scene/SceneEntity.h"

namespace ORNG {

	void MeshComponentManager::SortMeshIntoInstanceGroup(MeshComponent* comp, MeshAsset* asset) {

		int group_index = -1;

		// check if new entity can merge into already existing instance group
		for (int i = 0; i < m_instance_groups.size(); i++) {
			//if same data, shader and material, can be combined so instancing is possible
			if (m_instance_groups[i]->m_mesh_asset == asset
				&& m_instance_groups[i]->m_group_shader_id == comp->m_shader_id
				&& m_instance_groups[i]->m_materials == comp->m_materials) {
				group_index = i;
				break;
			}
		}

		if (group_index != -1) { // if instance group exists, place into
			// add mesh component's world transform into instance group for instanced rendering
			MeshInstanceGroup* group = m_instance_groups[group_index];
			group->AddMeshPtr(comp);
		}
		else { //else if instance group doesn't exist but mesh data exists, create group with existing data
			MeshInstanceGroup* group = new MeshInstanceGroup(asset, comp->m_shader_id, this, comp->m_materials);
			m_instance_groups.push_back(group);
			group->AddMeshPtr(comp);
		}

	}


	void MeshComponentManager::DeleteComponent(SceneEntity* p_entity) {

		if (!p_entity->GetComponent<MeshComponent>())
			return;

		MeshComponent* mesh = GetComponent(p_entity->GetID());
		MeshInstanceGroup* group = mesh->mp_instance_group;

		group->DeleteMeshPtr(mesh);

		// Unhook update callback
		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		p_transform->update_callbacks.erase(TransformComponent::CallbackType::MESH);


		m_mesh_components.erase(std::find_if(m_mesh_components.begin(), m_mesh_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == p_entity->GetID(); }));
		delete mesh;


	}


	MeshComponent* MeshComponentManager::AddComponent(SceneEntity* p_entity, MeshAsset* p_asset) {

		auto* p_existing_comp = GetComponent(p_entity->GetID());
		if (p_existing_comp) {
			OAR_CORE_WARN("Mesh component not added, entity '{0}' already has a mesh component", p_entity->name);
			return p_existing_comp;
		}
		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		auto* p_comp = new MeshComponent(p_entity, p_transform);
		for (auto* p_mat : p_asset->m_scene_materials) {
			p_comp->m_materials.push_back(p_mat);
		}

		// Sort into a group containing other meshes with same asset, material and shader for instanced rendering
		SortMeshIntoInstanceGroup(p_comp, p_asset);

		// Setup update callback, when transform is updated the transform buffers will update too
		p_comp->p_transform->update_callbacks[TransformComponent::CallbackType::MESH] = ([p_comp](TransformComponent::UpdateType) {
			p_comp->RequestTransformSSBOUpdate();
			});

		m_mesh_components.push_back(p_comp);

		return p_comp;
	}




	MeshComponent* MeshComponentManager::GetComponent(uint64_t entity_id) {
		auto it = std::find_if(m_mesh_components.begin(), m_mesh_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });
		return it == m_mesh_components.end() ? nullptr : *it;
	}




	void MeshComponentManager::OnLoad() {
		for (auto& group : m_instance_groups) {
			//Set materials
			for (auto* p_material : group->m_mesh_asset->m_scene_materials) {
				group->m_materials.push_back(p_material);
			}

			for (auto* p_mesh : group->m_instances) {
				p_mesh->m_materials = group->m_materials;
			}
			group->UpdateTransformSSBO();
		}
	}




	void MeshComponentManager::OnUnload() {
		for (auto* mesh : m_mesh_components) {
			delete mesh;
		}
		for (auto* group : m_instance_groups) {
			delete group;
		}
		m_mesh_components.clear();
		m_instance_groups.clear();
	}



	void MeshComponentManager::OnMeshAssetDeletion(MeshAsset* p_asset) {
		// Remove asset from all components using it
		for (int i = 0; i < m_instance_groups.size(); i++) {
			MeshInstanceGroup* group = m_instance_groups[i];

			if (group->m_mesh_asset == p_asset) {
				group->ClearMeshes();
				for (auto& mesh : group->m_instances) {
					mesh->mp_mesh_asset = nullptr;
				}

				// Delete all mesh instance groups using the asset as they cannot function without it
				m_instance_groups.erase(m_instance_groups.begin() + i);
				delete group;
			}
		}
	}

	void MeshComponentManager::OnMaterialDeletion(Material* p_material, Material* p_replacement_material) {
		for (int i = 0; i < m_instance_groups.size(); i++) {
			MeshInstanceGroup* group = m_instance_groups[i];

			std::vector<unsigned int> material_indices;

			// Replace material in group if it contains it
			for (int y = 0; y < group->m_materials.size(); y++) {
				const Material*& p_group_mat = group->m_materials[y];
				if (p_group_mat == p_material) {
					p_group_mat = p_replacement_material;
					material_indices.push_back(y);
				}
			}

			if (material_indices.empty())
				continue;

			// Replace material in mesh if it contains it
			for (auto* p_mesh_comp : group->m_instances) {
				for (auto valid_replacement_index : material_indices) {
					p_mesh_comp->m_materials[valid_replacement_index] = p_replacement_material;
				}

			}

		}
	}


	void MeshComponentManager::OnUpdate() {
		for (int i = 0; i < m_instance_groups.size(); i++) {
			auto* group = m_instance_groups[i];

			group->ProcessUpdates();

			// Check if group should be deleted
			if (group->m_instances.empty()) {
				delete group;
				m_instance_groups.erase(m_instance_groups.begin() + i);
			}
		}
	}
}