#include "WorldTransform.h"
#include "ExtraMath.h"
#include <glm/glm.hpp>


const double pi = atan(1) * 4;


WorldTransform::WorldTransform() {

}

void WorldTransform::SetScale(float scaleX, float scaleY, float scaleZ) {
	m_scale.x = scaleX;
	m_scale.y = scaleY;
	m_scale.z = scaleZ;

}

void WorldTransform::SetPosition(float x, float y, float z) {
	m_pos.x = x;
	m_pos.y = y;
	m_pos.z = z;
}

glm::fvec3 WorldTransform::GetPosition() const {
	return m_pos;
}


void WorldTransform::SetRotation(float x, float y, float z) {
	m_rotation.x = x;
	m_rotation.y = y;
	m_rotation.z = z;
}

void WorldTransform::Rotate(float x, float y, float z) {
	m_rotation.x += x;
	m_rotation.y += y;
	m_rotation.z += z;
}

glm::fmat4x4 WorldTransform::GetMatrix() const {

	glm::fmat4x4 rotMat = ExtraMath::Init3DRotateTransform(m_rotation.x, m_rotation.y, m_rotation.z);
	glm::fmat4x4 scaleMat = ExtraMath::Init3DScaleTransform(m_scale.x, m_scale.y, m_scale.z);
	glm::fmat4x4 transMat = ExtraMath::Init3DTranslationTransform(m_pos.x, m_pos.y, m_pos.z);


	glm::fmat4x4 worldTransMat = rotMat * scaleMat * transMat;

	return worldTransMat;
}
