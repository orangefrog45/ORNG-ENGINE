#include "pch/pch.h"
#include "components/ComponentManagers.h"
#include "scene/SceneEntity.h"

namespace ORNG {

	CameraComponent* CameraSystem::AddComponent(SceneEntity* p_entity) {
		auto* p_existing = GetComponent(p_entity->GetID());

		if (p_existing) {
			OAR_CORE_ERROR("Entity '{0}' already has a camera component, component not added", p_entity->name);
			return p_existing;
		}

		auto* p_comp = new CameraComponent(p_entity, this, p_entity->GetComponent<TransformComponent>());
		m_camera_components.push_back(p_comp);
	}


	CameraComponent* CameraSystem::GetComponent(uint64_t entity_id) {
		auto it = std::find_if(m_camera_components.begin(), m_camera_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == entity_id; });

		if (it == m_camera_components.end()) {
			return nullptr;
		}

		return *it;
	}

	void CameraSystem::DeleteComponent(SceneEntity* p_entity) {
		auto it = std::find_if(m_camera_components.begin(), m_camera_components.end(), [&](const auto& p_comp) {return p_comp->GetEntityHandle() == p_entity->GetID(); });

		if (it == m_camera_components.end()) {
			OAR_CORE_WARN("No camera component found in entity with id '{0}', DeleteComponent failed", p_entity->GetID());
			return;
		}

		m_camera_components.erase(it);
	}

	void CameraSystem::OnUnload() {
		for (auto* p_comp : m_camera_components) {
			delete p_comp;
		}

		m_camera_components.clear();
	}
}
