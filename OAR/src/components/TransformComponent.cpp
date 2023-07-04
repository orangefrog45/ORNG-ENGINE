#include "pch/pch.h"

#include "components/TransformComponent.h"
#include "util/ExtraMath.h"

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

		if (!mp_parent_transform)
			return std::array<glm::vec3, 3>{m_pos, m_scale, m_rotation};

		// Sum all parent transforms to get absolute position
		TransformComponent* p_parent_transform = mp_parent_transform;

		glm::vec3 abs_rotation = m_rotation;
		glm::vec3 abs_translation = m_pos;
		glm::vec3 abs_scale = m_scale;

		while (p_parent_transform) {
			abs_rotation += p_parent_transform->m_rotation;
			abs_translation += p_parent_transform->m_pos;
			abs_scale *= p_parent_transform->m_scale;

			p_parent_transform = p_parent_transform->mp_parent_transform;
		}


		/*glm::vec3 parent_abs_pos = abs_translation - m_pos;
		glm::vec3 transformed_pos = m_pos - parent_abs_pos;
		glm::vec3 parent_abs_rot = abs_rotation - m_rotation;
		glm::vec3 rotated_pos = glm::mat3(ExtraMath::Init3DRotateTransform(parent_abs_rot.x, parent_abs_rot.y, parent_abs_rot.z)) * transformed_pos;
		rotated_pos += parent_abs_pos;*/
		return std::array<glm::vec3, 3>{abs_translation, abs_scale, abs_rotation};
	}



	void TransformComponent::RebuildMatrix(UpdateType type) {

		if (m_is_absolute) {
			glm::mat4x4 rot_mat = ExtraMath::Init3DRotateTransform(m_rotation.x, m_rotation.y, m_rotation.z);
			glm::mat4x4 scale_mat = ExtraMath::Init3DScaleTransform(m_scale.x, m_scale.y, m_scale.z);
			glm::mat4x4 trans_mat = ExtraMath::Init3DTranslationTransform(m_pos.x, m_pos.y, m_pos.z);

			m_transform = trans_mat * rot_mat * scale_mat;
		}
		else {
			// Sum all parent transforms to get absolute position
			auto abs_transforms = GetAbsoluteTransforms();
			glm::vec3 abs_translation = abs_transforms[0];
			glm::vec3 abs_scale = abs_transforms[1];
			glm::vec3 abs_rotation = abs_transforms[2];

			glm::mat4x4 rot_mat = ExtraMath::Init3DRotateTransform(abs_rotation.x, abs_rotation.y, abs_rotation.z);
			glm::mat4x4 scale_mat = ExtraMath::Init3DScaleTransform(abs_scale.x, abs_scale.y, abs_scale.z);
			glm::mat4x4 trans_mat = ExtraMath::Init3DTranslationTransform(abs_translation.x, abs_translation.y, abs_translation.z);


			m_transform = trans_mat * rot_mat * scale_mat;

		}

		for (auto& it : update_callbacks) {
			it.second(type);
		}

		// Update child transforms as they are now inaccurate as the matrix has been rebuilt as a result of the transform changing
		for (auto* p_transform : m_child_transforms) {
			p_transform->RebuildMatrix(type);
		}
	}

}