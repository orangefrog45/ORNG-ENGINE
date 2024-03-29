#include "pch/pch.h"

#include "components/TransformComponent.h"
#include "util/ExtraMath.h"
#include "events/EventManager.h"
#include "scene/SceneEntity.h"



namespace ORNG {
	void TransformComponent2D::SetScale(float x, float y) {
		m_scale.x = x;
		m_scale.y = y;
	}

	void TransformComponent2D::SetOrientation(float rot) {
		m_rotation = rot;
	}

	void TransformComponent2D::SetPosition(float x, float y) {
		m_pos.x = x;
		m_pos.y = y;
	}

	glm::mat3 TransformComponent2D::GetMatrix() const {
		glm::mat3 rot_mat = ExtraMath::Init2DRotateTransform(m_rotation);
		glm::mat3 trans_mat = ExtraMath::Init2DTranslationTransform(m_pos.x, m_pos.y);
		glm::mat3 scale_mat = ExtraMath::Init2DScaleTransform(m_scale.x, m_scale.y);

		return scale_mat * rot_mat * trans_mat;
	}



	void TransformComponent::UpdateAbsTransforms() {
		TransformComponent* p_parent_transform = GetParent();

		m_abs_scale = m_scale;
		m_abs_orientation = m_orientation;

		if (!p_parent_transform || m_is_absolute) {
			m_abs_pos = m_pos;
			return;
		}

		while (p_parent_transform) {
			m_abs_orientation += p_parent_transform->m_orientation;
			m_abs_scale *= p_parent_transform->m_scale;

			p_parent_transform = p_parent_transform->GetParent();
		}

		m_abs_pos = GetParent()->GetMatrix() * glm::vec4(m_pos, 1.0);
	}


	void TransformComponent::LookAt(glm::vec3 t_pos, glm::vec3 t_up, glm::vec3 t_right) {
		glm::vec3 t_target = glm::normalize(t_pos - m_abs_pos);
		t_target = glm::normalize(t_target);
		t_right = glm::normalize(glm::cross(t_target, up));
		g_up = t_up;
		t_up = glm::normalize(glm::cross(t_right, t_target));

		glm::mat3 rotation_matrix = {
			t_right.x, t_right.y, t_right.z,
			t_up.x, t_up.y, t_up.z,
			-t_target.x, -t_target.y, -t_target.z };

		glm::vec3 euler_angles = glm::degrees(glm::eulerAngles(glm::quat_cast(rotation_matrix)));
		SetAbsoluteOrientation(euler_angles);
	}


	TransformComponent* TransformComponent::GetParent() {
		return m_parent_handle == entt::null ? nullptr : GetEntity()->GetRegistry()->try_get<TransformComponent>(m_parent_handle);
	}

	void TransformComponent::RebuildMatrix(UpdateType type) {
		ORNG_TRACY_PROFILE;

		auto* p_parent = GetParent();
		glm::mat3 rot_mat = ExtraMath::Init3DRotateTransform(m_orientation.x, m_orientation.y, m_orientation.z);
		if (m_is_absolute || !p_parent) {
			glm::mat3 scale_mat = ExtraMath::Init3DScaleTransform(m_scale.x, m_scale.y, m_scale.z);
			m_transform = glm::mat4(rot_mat * scale_mat);

			m_transform[3][0] = m_pos.x;
			m_transform[3][1] = m_pos.y;
			m_transform[3][2] = m_pos.z;
			m_transform[3][3] = 1.0;
		}
		else {
			auto p_s = p_parent->GetAbsScale();
			// Apply parent scaling to the position and scale to make up for not using the scale transform below
			glm::mat3 scale_mat = ExtraMath::Init3DScaleTransform(m_scale.x * p_s.x, m_scale.y * p_s.y, m_scale.z * p_s.z);

			// Undo scaling to prevent shearing
			glm::mat4 m1 = glm::mat3(glm::inverse(ExtraMath::Init3DScaleTransform(p_s.x, p_s.y, p_s.z)) * (rot_mat * scale_mat));
			m1[3][0] = m_pos.x;
			m1[3][1] = m_pos.y;
			m1[3][2] = m_pos.z;
			m1[3][3] = 1.0;

			m_transform = p_parent->GetMatrix() * m1;
		}

		forward = glm::normalize(glm::mat3(m_transform) * glm::vec3(0.0, 0.0, -1.0));
		right = glm::normalize(glm::cross(forward, g_up));
		up = glm::normalize(glm::cross(right, forward));

		UpdateAbsTransforms();

		if (GetEntity()) {
			Events::ECS_Event<TransformComponent> e_event{ Events::ECS_EventType::COMP_UPDATED, this, type };
			Events::EventManager::DispatchEvent(e_event);
		}
	}
}