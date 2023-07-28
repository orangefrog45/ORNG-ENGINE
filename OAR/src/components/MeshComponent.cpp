#include "pch/pch.h"

#include "components/MeshComponent.h"
#include "events/EventManager.h"

namespace ORNG {

	MeshComponent::MeshComponent(SceneEntity* p_entity, MeshAsset* p_asset) : Component(p_entity) {
		mp_mesh_asset = p_asset;
		for (auto* p_material : p_asset->GetSceneMaterials()) {
			m_materials.push_back(p_material);
		}
	};



	void MeshComponent::SetMeshAsset(MeshAsset* p_asset) {
		mp_mesh_asset = p_asset;
		m_materials.clear();

		for (auto* p_material : p_asset->GetSceneMaterials()) {
			m_materials.push_back(p_material);
		}
		DispatchUpdateEvent();

	};


	void MeshComponent::SetShaderID(unsigned int id) {
		m_shader_id = id;
		if (mp_mesh_asset)
			DispatchUpdateEvent();
	}

	void MeshComponent::SetMaterialID(unsigned int index, const Material* p_material) {
		if (index >= m_materials.size()) {
			ORNG_CORE_ERROR("Material ID not set for mesh component, index out of range");
			BREAKPOINT;
		}

		m_materials[index] = p_material;
		if (mp_mesh_asset)
			DispatchUpdateEvent();
	}

	void MeshComponent::DispatchUpdateEvent() {
		Events::ECS_Event<MeshComponent> e_event;
		e_event.affected_components.push_back(this);
		e_event.event_type = Events::ECS_EventType::COMP_UPDATED;

		Events::EventManager::DispatchEvent(e_event);
		//m_transform_update_flag = true;
		//if (mp_mesh_asset && mp_mesh_asset->GetLoadStatus() == true) mp_instance_group->ActivateFlagSubUpdateWorldMatBuffer();
	}


}