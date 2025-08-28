#include "pch/pch.h"

#include "components/MeshComponent.h"
#include "events/EventManager.h"

namespace ORNG {
	MeshComponent::MeshComponent(SceneEntity* p_entity) : Component(p_entity) {
	};


	MeshComponent::MeshComponent(SceneEntity* p_entity, MeshAsset* p_asset) : Component(p_entity) {
		mp_mesh_asset = p_asset;
	};

	MeshComponent::MeshComponent(SceneEntity* p_entity, MeshAsset* p_asset, std::vector<const Material*>&& materials) : Component(p_entity),
	m_materials(std::move(materials)), mp_mesh_asset(p_asset) {}


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
		DispatchUpdateEvent();
	}

	void MeshComponent::DispatchUpdateEvent() {
		Events::ECS_Event<MeshComponent> e_event{ Events::ECS_EventType::COMP_UPDATED, this };
		Events::EventManager::DispatchEvent(e_event);
	}
}
