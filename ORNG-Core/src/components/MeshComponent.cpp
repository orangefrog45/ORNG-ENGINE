#include "pch/pch.h"

#include "components/MeshComponent.h"
#include "events/EventManager.h"

namespace ORNG {

	MeshComponent::MeshComponent(SceneEntity* p_entity, MeshAsset* p_asset) : Component(p_entity) {
		if (!p_asset)
			return;
		mp_mesh_asset = p_asset;

	};



	void MeshComponent::SetMeshAsset(MeshAsset* p_asset) {
		mp_mesh_asset = p_asset;
		m_materials.clear();
		DispatchUpdateEvent();
	};


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
		e_event.affected_components[0] = this;
		e_event.affected_entities[0] = GetEntity();
		e_event.event_type = Events::ECS_EventType::COMP_UPDATED;

		Events::EventManager::DispatchEvent(e_event);
	}


}