#include "pch/pch.h"

#include "components/TransformComponent.h"
#include "util/ExtraMath.h"
#include "events/EventManager.h"
#include "scene/SceneEntity.h"

namespace ORNG {
	void TransformComponent::UpdateAbsTransforms() {
		TransformComponent* p_parent = GetParent();

		if (m_is_absolute || !p_parent) {
			m_abs_scale = m_scale;
			m_abs_orientation = m_orientation;
			m_abs_pos = m_pos;
		}
		else {
			m_abs_scale = p_parent->m_abs_scale * m_scale;
			m_abs_orientation = p_parent->m_abs_orientation * m_orientation;
			m_abs_pos = lml::vec3(p_parent->GetMatrix() * lml::vec4(m_pos, 1.0f));
		}

		forward = m_abs_orientation * lml::vec3{ 0.0f, 0.0f, -1.0f };
		up = m_abs_orientation * lml::vec3{ 0.0f, 1.0f, 0.0f };
		right = m_abs_orientation * lml::vec3{ 1.0f, 0.0f, 0.0f };
	}


	void TransformComponent::LookAt(lml::vec3 t_pos, lml::vec3 t_up) {
		lml::vec3 dir = lml::normalize(t_pos - m_abs_pos);
		if (lml::length(dir) < 0.0001f)
			return;

		lml::quat q = lml::quatLookAt(dir, lml::normalize(t_up));
		if (lml::dot(q, q) > 0.0001f)
			SetAbsOrientationQuat(q);
	}


	TransformComponent* TransformComponent::GetParent() {
		return m_parent_handle == entt::null ? nullptr : &GetEntity()->GetRegistry()->get<TransformComponent>(m_parent_handle);
	}

	void TransformComponent::RebuildMatrix(UpdateType type) {
		UpdateAbsTransforms();

		m_transform = lml::mat4_cast(m_abs_orientation);

		m_transform[0][0] *= m_abs_scale.x;
		m_transform[0][1] *= m_abs_scale.x;
		m_transform[0][2] *= m_abs_scale.x;

		m_transform[1][0] *= m_abs_scale.y;
		m_transform[1][1] *= m_abs_scale.y;
		m_transform[1][2] *= m_abs_scale.y;

		m_transform[2][0] *= m_abs_scale.z;
		m_transform[2][1] *= m_abs_scale.z;
		m_transform[2][2] *= m_abs_scale.z;

		m_transform[3][0] = m_abs_pos.x;
		m_transform[3][1] = m_abs_pos.y;
		m_transform[3][2] = m_abs_pos.z;
		m_transform[3][3] = 1.f;

		if (GetEntity()) {
			Events::ECS_Event<TransformComponent> e_event{ Events::ECS_EventType::COMP_UPDATED, this, type };
			Events::EventManager::DispatchEvent(e_event);
		}
	}
}
