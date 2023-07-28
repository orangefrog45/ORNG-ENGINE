#include "pch/pch.h"

#include "components/TransformComponent.h"
#include "util/ExtraMath.h"
#include "events/EventManager.h"

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


	std::array<glm::vec3, 3> TransformComponent::GetAbsoluteTransforms() const {

		//if (!mp_parent_transform)
		return std::array<glm::vec3, 3>{m_pos, m_scale, m_orientation};

		// Sum all parent transforms to get absolute position
		/*TransformComponent* p_parent_transform = mp_parent_transform;

		glm::vec3 abs_rotation = m_orientation;
		glm::vec3 abs_translation = m_pos;
		glm::vec3 abs_scale = m_scale;

		while (p_parent_transform) {
			abs_rotation += p_parent_transform->m_orientation;
			abs_translation += p_parent_transform->m_pos;
			abs_scale *= p_parent_transform->m_scale;

			//p_parent_transform = p_parent_transform->mp_parent_transform;
		}

		glm::vec3 parent_abs_translation = abs_translation - m_pos;
		glm::vec3 parent_abs_rot = abs_rotation - m_orientation;
		glm::vec3 parent_abs_scale = abs_scale / m_scale;
		glm::vec3 rotation_offset = glm::mat3(ExtraMath::Init3DRotateTransform(parent_abs_rot.x, parent_abs_rot.y, parent_abs_rot.z)) * m_pos; // rotate around transformed origin (entity)
		rotation_offset *= glm::vec3(parent_abs_scale.x, parent_abs_scale.y, parent_abs_scale.z);
		return std::array<glm::vec3, 3>{parent_abs_translation + rotation_offset, abs_scale, abs_rotation};*/
	}



	void TransformComponent::RebuildMatrix(UpdateType type) {

		glm::mat4x4 rot_mat = ExtraMath::Init3DRotateTransform(m_orientation.x, m_orientation.y, m_orientation.z);
		glm::mat4x4 scale_mat = ExtraMath::Init3DScaleTransform(m_scale.x, m_scale.y, m_scale.z);
		glm::mat4x4 trans_mat = ExtraMath::Init3DTranslationTransform(m_pos.x, m_pos.y, m_pos.z);

		//if (m_is_absolute || !mp_parent_transform)
		m_transform = trans_mat * rot_mat * scale_mat;
		//else
			//m_transform = mp_parent_transform->GetMatrix() * (trans_mat * rot_mat * scale_mat);

		if (GetEntity()) {
			Events::ECS_Event<TransformComponent> e_event;
			e_event.affected_components.push_back(this);
			e_event.event_type = Events::ECS_EventType::COMP_UPDATED;
			e_event.sub_event_type = type;
			Events::EventManager::DispatchEvent(e_event);
		}

		// Update child transforms as they are now inaccurate as the matrix has been rebuilt as a result of the transform changing
		//for (auto* p_transform : m_child_transforms) {
			//p_transform->RebuildMatrix(type);
		//}
	}

}