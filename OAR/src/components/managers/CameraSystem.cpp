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

		auto* p_transform = p_entity->GetComponent<TransformComponent>();
		auto* p_comp = new CameraComponent(p_entity, this, p_transform);

		p_transform->update_callbacks[TransformComponent::CallbackType::CAMERA] = [p_transform, p_comp](TransformComponent::UpdateType type) {
			glm::vec3 rot = p_transform->GetAbsoluteTransforms()[2];
			p_comp->target = glm::normalize(glm::mat3(ExtraMath::Init3DRotateTransform(rot.x, rot.y, rot.z)) * glm::vec3(0, 0, -1.f)); // temporary fix to spin issue
		};

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

		if (it == m_camera_components.end())
			return;


		m_camera_components.erase(it);
	}

	void CameraSystem::OnUnload() {
		for (auto* p_comp : m_camera_components) {
			delete p_comp;
		}

		m_camera_components.clear();
	}
}
