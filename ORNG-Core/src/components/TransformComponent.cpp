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



	std::array<glm::vec3, 3> TransformComponent::GetAbsoluteTransforms() {
		// Sum all parent transforms to get absolute position
		TransformComponent* p_parent_transform = GetParent();

		glm::vec3 abs_translation = m_pos;
		glm::vec3 abs_scale = m_scale;
		glm::vec3 abs_rotation = m_orientation;

		if (!p_parent_transform || m_is_absolute)
			return std::array<glm::vec3, 3>{abs_translation, abs_scale, abs_rotation};

		while (p_parent_transform) {
			abs_rotation += p_parent_transform->m_orientation;
			abs_translation += p_parent_transform->m_pos;
			abs_scale *= p_parent_transform->m_scale;

			p_parent_transform = p_parent_transform->GetParent();
		}


		glm::vec3 pos = GetParent()->GetMatrix() * glm::vec4(m_pos, 1.0);
		return std::array<glm::vec3, 3>{pos, abs_scale, abs_rotation};
	}


	void TransformComponent::LookAt(glm::vec3 t_pos, glm::vec3 t_up, glm::vec3 t_right) {
		glm::vec3 abs_pos = GetAbsoluteTransforms()[0];
		glm::vec3 t_target = glm::normalize(t_pos - abs_pos);
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
		auto* p_parent = GetParent();
		glm::mat4x4 rot_mat = ExtraMath::Init3DRotateTransform(m_orientation.x, m_orientation.y, m_orientation.z);
		if (m_is_absolute || !p_parent) {
			glm::mat4x4 scale_mat = ExtraMath::Init3DScaleTransform(m_scale.x, m_scale.y, m_scale.z);
			glm::mat4x4 trans_mat = ExtraMath::Init3DTranslationTransform(m_pos.x, m_pos.y, m_pos.z);
			m_transform = trans_mat * rot_mat * scale_mat;
		}
		else {
			auto transforms = p_parent->GetAbsoluteTransforms();
			auto s = transforms[1];
			auto t = transforms[0];
			auto r = transforms[2];

			// Apply parent scaling to the position and scale to make up for not using the scale transform below
			glm::mat4x4 scale_mat = ExtraMath::Init3DScaleTransform(m_scale.x * s.x, m_scale.y * s.y, m_scale.z * s.z);
			glm::mat4x4 trans_mat = ExtraMath::Init3DTranslationTransform(m_pos.x * s.x, m_pos.y * s.y, m_pos.z * s.z);

			// Undo scaling to prevent shearing
			// TODO: OPTIMIZE THIS
			m_transform = p_parent->GetMatrix() * glm::inverse(ExtraMath::Init3DScaleTransform(s.x, s.y, s.z)) * (trans_mat * rot_mat * scale_mat);
		}


		glm::mat3 rot_mat_new{ m_transform };

		forward = glm::normalize(rot_mat_new * glm::vec3(0.0, 0.0, -1.0));
		right = glm::normalize(glm::cross(forward, g_up));
		up = glm::normalize(glm::cross(right, forward));

		if (GetEntity()) {
			Events::ECS_Event<TransformComponent> e_event{ Events::ECS_EventType::COMP_UPDATED, this, type };
			Events::EventManager::DispatchEvent(e_event);
		}
	}
}