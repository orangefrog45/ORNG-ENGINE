#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "scene/SceneEntity.h"
#include "assets/AssetManager.h"
#include "core/CodedAssets.h"
#include "scene/MeshInstanceGroup.h"

namespace ORNG {
	static void OnMeshComponentAdd(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<MeshComponent>(registry, entity, Events::ECS_EventType::COMP_ADDED);
	}

	static void OnMeshComponentDestroy(entt::registry& registry, entt::entity entity) {
		ComponentSystem::DispatchComponentEvent<MeshComponent>(registry, entity, Events::ECS_EventType::COMP_DELETED);
	}

	void MeshInstancingSystem::SortMeshIntoInstanceGroup(MeshComponent* comp) {
		if (!comp->mp_mesh_asset)
			return;

		if (comp->mp_instance_group) { // Remove first if it has an instance group
			comp->mp_instance_group->DeleteMeshPtr(comp);
		}

		int group_index = -1;

		// check if new entity can merge into already existing instance group
		for (int i = 0; i < m_instance_groups.size(); i++) {
			//if same data and material, can be combined so instancing is possible
			if (m_instance_groups[i]->m_mesh_asset == comp->mp_mesh_asset
				&& (m_instance_groups[i]->m_materials == comp->m_materials || comp->m_materials.empty())) {
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
			std::vector<const Material*> material_vec;
			if (comp->m_materials.empty()) {
				for (int i = 0; i < comp->mp_mesh_asset->num_materials; i++) {
					material_vec.push_back(AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID));
				}
			}
			else {
				material_vec = comp->m_materials;
			}

			MeshInstanceGroup* group = new MeshInstanceGroup(comp->mp_mesh_asset, this, material_vec);
			m_instance_groups.push_back(group);
			group->AddMeshPtr(comp);
		}
	}



	MeshInstancingSystem::MeshInstancingSystem(entt::registry* p_registry, uint64_t scene_uuid) : ComponentSystem(p_registry, scene_uuid) {
		// Setup event listeners
		m_transform_listener.scene_id = scene_uuid;
		m_transform_listener.OnEvent = [this](const Events::ECS_Event<TransformComponent>& t_event) {
			if (mp_registry->valid(entt::entity(t_event.affected_components[0]->GetEntity()->GetEnttHandle())))
				OnTransformEvent(t_event);
			};

		// Instance group handling
		m_mesh_listener.scene_id = scene_uuid;
		m_mesh_listener.OnEvent = [this](const Events::ECS_Event<MeshComponent>& t_event) {
			OnMeshEvent(t_event);
			};

		m_asset_listener.OnEvent = [this](const Events::AssetEvent& t_event) {
			if (t_event.event_type == Events::AssetEventType::MATERIAL_DELETED) {
				OnMaterialDeletion(reinterpret_cast<Material*>(t_event.data_payload));
			}
			else if (t_event.event_type == Events::AssetEventType::MESH_DELETED) {
				OnMeshAssetDeletion(reinterpret_cast<MeshAsset*>(t_event.data_payload));
			}
			};

		Events::EventManager::RegisterListener(m_mesh_listener);
		Events::EventManager::RegisterListener(m_asset_listener);
		Events::EventManager::RegisterListener(m_transform_listener);
	};



	void MeshInstancingSystem::OnMeshEvent(const Events::ECS_Event<MeshComponent>& t_event) {
		switch (t_event.event_type) {
		case Events::ECS_EventType::COMP_ADDED:
			if (!t_event.affected_components[0]->mp_mesh_asset) {
				t_event.affected_components[0]->mp_mesh_asset = AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID);
				t_event.affected_components[0]->m_materials.push_back(AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID));
			}

			SortMeshIntoInstanceGroup(t_event.affected_components[0]);
			break;
		case Events::ECS_EventType::COMP_UPDATED:
			SortMeshIntoInstanceGroup(t_event.affected_components[0]);
			break;
		case Events::ECS_EventType::COMP_DELETED:
			t_event.affected_components[0]->mp_instance_group->DeleteMeshPtr(t_event.affected_components[0]);
			break;
		}
	}



	void MeshInstancingSystem::OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event) {
		// Whenever a transform component changes, check if it has a mesh, if so then update the transform buffer of the instance group holding it.
		if (t_event.event_type != Events::ECS_EventType::COMP_UPDATED)
			return;

		if (auto* meshc = t_event.affected_components[0]->GetEntity()->GetComponent<MeshComponent>()) {
			meshc->mp_instance_group->ActivateFlagSubUpdateWorldMatBuffer();
			meshc->m_transform_update_flag = true;
		}
	}





	void MeshInstancingSystem::OnLoad() {
		mp_registry->on_construct<MeshComponent>().connect<&OnMeshComponentAdd>();
		mp_registry->on_destroy<MeshComponent>().connect<&OnMeshComponentDestroy>();

		for (auto& group : m_instance_groups) {
			//Set materials
			for (int i = 0; i < group->m_mesh_asset->num_materials; i++) {
				group->m_materials.push_back(AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID));
			}

			for (auto* p_mesh : group->m_instances) {
				p_mesh->m_materials = group->m_materials;
			}
			group->UpdateTransformSSBO();
		}
	}




	void MeshInstancingSystem::OnUnload() {
		for (auto* group : m_instance_groups) {
			delete group;
		}
		m_instance_groups.clear();
	}



	void MeshInstancingSystem::OnMeshAssetDeletion(MeshAsset* p_asset) {
		// Remove asset from all components using it
		for (auto [entity, mesh] : mp_registry->view<MeshComponent>().each()) {
			if (mesh.GetMeshData() == p_asset)
				mesh.SetMeshAsset(AssetManager::GetAsset<MeshAsset>(ORNG_BASE_MESH_ID));
		}

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

	void MeshInstancingSystem::OnMaterialDeletion(Material* p_material) {
		for (int i = 0; i < m_instance_groups.size(); i++) {
			MeshInstanceGroup* group = m_instance_groups[i];

			std::vector<unsigned int> material_indices;

			// Replace material in group if it contains it
			for (int y = 0; y < group->m_materials.size(); y++) {
				const Material*& p_group_mat = group->m_materials[y];
				if (p_group_mat == p_material) {
					p_group_mat = AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID);
					material_indices.push_back(y);
				}
			}

			if (material_indices.empty())
				continue;

			// Replace material in mesh if it contains it
			for (auto* p_mesh_comp : group->m_instances) {
				for (auto valid_replacement_index : material_indices) {
					p_mesh_comp->m_materials[valid_replacement_index] = AssetManager::GetAsset<Material>(ORNG_BASE_MATERIAL_ID);
				}
			}
		}
	}


	void MeshInstancingSystem::OnUpdate() {
		for (int i = 0; i < m_instance_groups.size(); i++) {
			auto* group = m_instance_groups[i];

			group->ProcessUpdates();

			// Check if group should be deleted
			if (group->m_instances.empty()) {
				group->ProcessUpdates();
				delete group;
				m_instance_groups.erase(m_instance_groups.begin() + i);
				i--;
			}
		}
	}
}