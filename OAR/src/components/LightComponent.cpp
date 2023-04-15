#include "SceneEntity.h"
#include "LightComponent.h"
#include "MeshComponent.h"
#include <glm/gtx/transform.hpp>


void SpotLightComponent::SetLightDirection(float i, float j, float k) {
	m_light_direction_vec = glm::normalize(glm::fvec3(i, j, k));
	salt_m_mesh_visual->SetRotation(glm::degrees(sinf(j)) - 90.0f, -glm::degrees(atan2f(k, i)) - 90.0f, 0.0f);

	if (salt_shadows_enabled) UpdateLightTransform();
}

SpotLightComponent::SpotLightComponent(unsigned int entity_id) : PointLightComponent(entity_id)
{
	SetMaxDistance(96.0f);
	SetAttenuation(1.0f, 0.001f, 0.0001f);
	if (salt_shadows_enabled) UpdateLightTransform();
};

void SpotLightComponent::UpdateLightTransform() {
	float z_near = 0.1f;
	float z_far = GetMaxDistance();
	auto light_perspective = glm::perspective(glm::degrees(acosf(aperture)) + 3.0f, 1.0f, z_near, z_far);
	auto spot_light_view = glm::lookAt(GetWorldTransform().GetPosition(), GetWorldTransform().GetPosition() + m_light_direction_vec, glm::vec3(0.0f, 1.0f, 0.0f));


	salt_m_light_transforms[0] = glm::fmat4(light_perspective * spot_light_view);
}

void SpotLightComponent::SetPosition(const float x, const float y, const float z) {
	if (salt_m_mesh_visual != nullptr) {
		salt_transform.SetPosition(x, y, z);
		salt_m_mesh_visual->SetPosition(x, y, z);
	}

	if (salt_shadows_enabled) UpdateLightTransform();
};

PointLightComponent::PointLightComponent(unsigned int entity_id) : BaseLight(entity_id)
{
	salt_m_light_transforms.reserve(6);
	UpdateLightTransforms();
};

void PointLightComponent::UpdateLightTransforms() {
	float z_near = 0.1f;
	float z_far = GetMaxDistance();
	glm::fmat4 light_perspective = glm::perspective(45.0f, 1.0f, z_near, z_far);
	glm::fvec3 pos = GetWorldTransform().GetPosition();
	glm::fmat4 light_view_pos_x = glm::lookAt(pos, pos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::fmat4 light_view_neg_x = glm::lookAt(pos, pos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::fmat4 light_view_pos_y = glm::lookAt(pos, pos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
	glm::fmat4 light_view_neg_y = glm::lookAt(pos, pos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::fmat4 light_view_pos_z = glm::lookAt(pos, pos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::fmat4 light_view_neg_z = glm::lookAt(pos, pos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	salt_m_light_transforms.clear();
	salt_m_light_transforms.reserve(6);
	salt_m_light_transforms.emplace_back(light_perspective * light_view_pos_x);
	salt_m_light_transforms.emplace_back(light_perspective * light_view_neg_x);
	salt_m_light_transforms.emplace_back(light_perspective * light_view_pos_y);
	salt_m_light_transforms.emplace_back(light_perspective * light_view_neg_y);
	salt_m_light_transforms.emplace_back(light_perspective * light_view_pos_z);
	salt_m_light_transforms.emplace_back(light_perspective * light_view_neg_z);
}

void PointLightComponent::SetPosition(const float x, const float y, const float z)
{
	if (salt_m_mesh_visual != nullptr) {
		salt_transform.SetPosition(x, y, z); salt_m_mesh_visual->SetPosition(x, y, z);
	}
	UpdateLightTransforms();
};





void DirectionalLightComponent::SetLightDirection(const glm::fvec3& dir)
{
	salt_light_direction = dir;

	glm::mat4 light_perspective = glm::ortho(-70.0f, 70.0f, -70.0f, 70.0f, 0.0001f, 200.f);
	glm::mat4 light_view = glm::lookAt(glm::normalize(dir) * 80.0f, glm::fvec3(0), glm::vec3(0.0f, 1.0f, 0.0f));

	salt_mp_light_transform_matrix = glm::fmat4(light_perspective * light_view);

}

DirectionalLightComponent::DirectionalLightComponent(unsigned int entity_id) : BaseLight(entity_id) {
	SetDiffuseIntensity(1.0f);
	SetColor(0.922f, 0.985f, 0.875f);

	auto light_perspective = glm::ortho(-70.0f, 70.0f, -70.0f, 70.0f, 0.0001f, 200.f);
	glm::mat4 light_view = glm::lookAt(glm::normalize(GetLightDirection()) * 80.0f, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	salt_mp_light_transform_matrix = glm::fmat4(light_perspective * light_view);
}