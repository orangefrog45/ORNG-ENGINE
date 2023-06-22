#include "pch/pch.h"
#include "components/ComponentManagers.h"
#include "scene/SceneEntity.h"

namespace ORNG {

	TransformComponent* TransformComponentManager::GetComponent(unsigned long entity_id) {
		auto it = std::find_if(m_transform_components.begin(), m_transform_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });

		return it == m_transform_components.end() ? nullptr : *it;
	}




	TransformComponent* TransformComponentManager::AddComponent(SceneEntity* p_entity) {
		if (GetComponent(p_entity->GetID())) {
			OAR_CORE_WARN("Transform component not added, entity '{0}' already has a transform component", p_entity->name);
			return nullptr;
		}

		TransformComponent* comp = new TransformComponent(p_entity);
		m_transform_components.push_back(comp);
		return comp;
	}




	void TransformComponentManager::OnUnload() {
		for (auto* p_transform : m_transform_components) {
			delete p_transform;
		}
	}



}