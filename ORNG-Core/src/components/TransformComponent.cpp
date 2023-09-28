#include "pch/pch.h"

#include "components/TransformComponent.h"
#include "util/ExtraMath.h"
#include "events/EventManager.h"
#include "glm/glm/gtc/quaternion.hpp"
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



	glm::vec3 TransformComponent::GetPosition() const {
		return m_pos;
	}


	std::array<glm::vec3, 3> TransformComponent::GetAbsoluteTransforms() {

		// Sum all parent transforms to get absolute position
		TransformComponent* p_parent_transform = GetParent();

		glm::vec3 abs_rotation = m_orientation;
		glm::vec3 abs_translation = m_pos;
		glm::vec3 abs_scale = m_scale;

		if (!p_parent_transform || m_is_absolute)
			return std::array<glm::vec3, 3>{abs_translation, abs_scale, abs_rotation};

		while (p_parent_transform) {
			abs_rotation += p_parent_transform->m_orientation;
			abs_translation += p_parent_transform->m_pos;
			abs_scale *= p_parent_transform->m_scale;

			p_parent_transform = p_parent_transform->GetParent();
		}

		glm::vec3 parent_abs_translation = abs_translation - m_pos;
		glm::vec3 parent_abs_rot = abs_rotation - m_orientation;
		glm::vec3 parent_abs_scale = abs_scale / m_scale;
		glm::vec3 rotation_offset = glm::mat3(ExtraMath::Init3DRotateTransform(parent_abs_rot.x, parent_abs_rot.y, parent_abs_rot.z)) * m_pos;
		rotation_offset *= glm::vec3(parent_abs_scale.x, parent_abs_scale.y, parent_abs_scale.z);
		return std::array<glm::vec3, 3>{parent_abs_translation + rotation_offset, abs_scale, abs_rotation};
	}

	/*
		GLM_FUNC_QUALIFIER mat<4, 4, T, Q> lookAtRH(vec<3, T, Q> const& eye, vec<3, T, Q> const& center, vec<3, T, Q> const& up)
	{
		vec<3, T, Q> const f(normalize(center - eye));
		vec<3, T, Q> const s(normalize(cross(f, up)));
		vec<3, T, Q> const u(cross(s, f));

		mat<4, 4, T, Q> Result(1);
		Result[0][0] = s.x;
		Result[1][0] = s.y;
		Result[2][0] = s.z;
		Result[0][1] = u.x;
		Result[1][1] = u.y;
		Result[2][1] = u.z;
		Result[0][2] =-f.x;
		Result[1][2] =-f.y;
		Result[2][2] =-f.z;
		Result[3][0] =-dot(s, eye);
		Result[3][1] =-dot(u, eye);
		Result[3][2] = dot(f, eye);
	*/

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

		glm::mat4x4 rot_mat = ExtraMath::Init3DRotateTransform(m_orientation.x, m_orientation.y, m_orientation.z);
		glm::mat4x4 scale_mat = ExtraMath::Init3DScaleTransform(m_scale.x, m_scale.y, m_scale.z);
		glm::mat4x4 trans_mat = ExtraMath::Init3DTranslationTransform(m_pos.x, m_pos.y, m_pos.z);
		auto* p_parent = GetParent();
		if (m_is_absolute || !p_parent)
			m_transform = trans_mat * rot_mat * scale_mat;
		else
			m_transform = p_parent->GetMatrix() * (trans_mat * rot_mat * scale_mat);
		glm::mat3 rot_mat_new{m_transform};

		forward = glm::normalize(rot_mat_new * glm::vec3(0.0, 0.0, -1.0));
		right = glm::normalize(glm::cross(forward, g_up));
		up = glm::normalize(glm::cross(right, forward));

		if (GetEntity()) {
			Events::ECS_Event<TransformComponent> e_event;
			e_event.affected_components[0] = this;
			e_event.event_type = Events::ECS_EventType::COMP_UPDATED;
			e_event.sub_event_type = type;
			Events::EventManager::DispatchEvent(e_event);
		}
	}

}