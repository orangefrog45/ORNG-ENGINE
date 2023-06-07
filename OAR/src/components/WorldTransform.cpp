#include "pch/pch.h"

#include "components/WorldTransform.h"
#include "util/ExtraMath.h"

namespace ORNG {

	void WorldTransform2D::SetScale(float x, float y) {
		m_scale.x = x;
		m_scale.y = y;
	}

	void WorldTransform2D::SetOrientation(float rot) {
		m_rotation = rot;
	}

	void WorldTransform2D::SetPosition(float x, float y) {
		m_pos.x = x;
		m_pos.y = y;
	}

	glm::mat3 WorldTransform2D::GetMatrix() const {
		glm::mat3 rot_mat = ExtraMath::Init2DRotateTransform(m_rotation);
		glm::mat3 trans_mat = ExtraMath::Init2DTranslationTransform(m_pos.x, m_pos.y);
		glm::mat3 scale_mat = ExtraMath::Init2DScaleTransform(m_scale.x, m_scale.y);

		return scale_mat * rot_mat * trans_mat;
	}

	void WorldTransform::SetScale(float scaleX, float scaleY, float scaleZ) {
		m_scale.x = scaleX;
		m_scale.y = scaleY;
		m_scale.z = scaleZ;
		RebuildMatrix();

	}

	void WorldTransform::SetPosition(float x, float y, float z) {
		m_pos.x = x;
		m_pos.y = y;
		m_pos.z = z;
		RebuildMatrix();
	}


	glm::vec3 WorldTransform::GetPosition() const {
		return m_pos;
	}

	glm::mat4 WorldTransform::GetRotationMatrix() const {
		return ExtraMath::Init3DRotateTransform(m_rotation.x, m_rotation.y, m_rotation.z);
	}

	glm::mat4 WorldTransform::GetScaleMatrix() const {
		return ExtraMath::Init3DScaleTransform(m_scale.x, m_scale.y, m_scale.z);
	}

	glm::mat4 WorldTransform::GetTranslationMatrix() const {
		return ExtraMath::Init3DTranslationTransform(m_pos.x, m_pos.y, m_pos.z);
	}

	void WorldTransform::SetOrientation(float x, float y, float z) {
		m_rotation.x = x;
		m_rotation.y = y;
		m_rotation.z = z;
		RebuildMatrix();
	}

	void WorldTransform::RebuildMatrix() {
		glm::mat4x4 rot_mat = ExtraMath::Init3DRotateTransform(m_rotation.x, m_rotation.y, m_rotation.z);
		glm::mat4x4 scale_mat = ExtraMath::Init3DScaleTransform(m_scale.x, m_scale.y, m_scale.z);
		glm::mat4x4 trans_mat = ExtraMath::Init3DTranslationTransform(m_pos.x, m_pos.y, m_pos.z);


		m_transform = trans_mat * rot_mat * scale_mat;
	}

	const glm::mat4x4& WorldTransform::GetMatrix() const {
		return m_transform;
	}
}