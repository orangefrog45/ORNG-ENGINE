#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "scene/SceneEntity.h"

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
			//if same data, shader and material, can be combined so instancing is possible
			if (m_instance_groups[i]->m_mesh_asset == comp->mp_mesh_asset
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
			MeshInstanceGroup* group = new MeshInstanceGroup(comp->mp_mesh_asset, this, comp->m_materials);
			m_instance_groups.push_back(group);
			group->AddMeshPtr(comp);
		}

	}



	MeshInstancingSystem::MeshInstancingSystem(entt::registry* p_registry, uint64_t scene_uuid) :ComponentSystem(scene_uuid), mp_registry(p_registry) {
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

		Events::EventManager::RegisterListener(m_mesh_listener);
		Events::EventManager::RegisterListener(m_transform_listener);


	};



	void MeshInstancingSystem::OnMeshEvent(const Events::ECS_Event<MeshComponent>& t_event) {
		switch (t_event.event_type) {
		case Events::ECS_EventType::COMP_ADDED:
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
			for (auto* p_material : group->m_mesh_asset->m_scene_materials) {
				group->m_materials.push_back(p_material);
			}

			for (auto* p_mesh : group->m_instances) {
				p_mesh->m_materials = group->m_materials;
			}
			group->UpdateTransformSSBO();
		}
	}




	void MeshInstancingSystem::OnUnload() {
		mp_registry->clear<MeshComponent>();
		for (auto* group : m_instance_groups) {
			delete group;
		}
		m_instance_groups.clear();
	}



	void MeshInstancingSystem::OnMeshAssetDeletion(MeshAsset* p_asset) {
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

	void MeshInstancingSystem::OnMaterialDeletion(Material* p_material, Material* p_replacement_material) {
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


	void MeshInstancingSystem::OnUpdate() {
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