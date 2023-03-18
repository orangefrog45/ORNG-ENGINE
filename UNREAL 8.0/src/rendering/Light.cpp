#include "SceneEntity.h"
#include "LightComponent.h"
#include <glm/gtx/transform.hpp>


void SpotLightComponent::SetLightDirection(float i, float j, float k) {
	m_light_direction_vec = glm::normalize(glm::fvec3(i, j, k));
	m_mesh_visual->SetRotation(glm::degrees(sinf(j)) - 90.0f, -glm::degrees(atan2f(k, i)) - 90.0f, 0.0f);

	if (shadows_enabled) UpdateLightTransform();
}

SpotLightComponent::SpotLightComponent(unsigned int entity_id) : PointLightComponent(entity_id)
{
	SetMaxDistance(480.0f);
	SetAttenuation(0.1f, 0.005f, 0.001f);
	if (shadows_enabled) UpdateLightTransform();
};

void SpotLightComponent::UpdateLightTransform() {
	float z_near = 0.1f;
	float z_far = GetMaxDistance();
	auto light_perspective = glm::perspective(glm::degrees(acosf(aperture)) + 3.0f, 1.0f, z_near, z_far);
	auto spot_light_view = glm::lookAt(GetWorldTransform().GetPosition(), m_light_direction_vec * 50.0f, glm::vec3(0.0f, 1.0f, 0.0f));


	mp_light_transform_matrix = glm::fmat4(light_perspective * spot_light_view);
}

void SpotLightComponent::SetPosition(const float x, const float y, const float z) {
	if (m_mesh_visual != nullptr) {
		transform.SetPosition(x, y, z);
		m_mesh_visual->SetPosition(x, y, z);
	}

	if (shadows_enabled) UpdateLightTransform();
};



void DirectionalLightComponent::SetLightDirection(const glm::fvec3& dir)
{
	light_direction = dir;

	auto light_perspective = glm::ortho(-70.0f, 70.0f, -70.0f, 70.0f, 0.0001f, 200.f);
	glm::mat4 light_view = glm::lookAt(glm::normalize(dir) * 40.0f, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	mp_light_transform_matrix = glm::fmat4(light_perspective * light_view);

}

DirectionalLightComponent::DirectionalLightComponent(unsigned int entity_id) : BaseLight(entity_id) {
	SetDiffuseIntensity(1.0f);
	SetColor(0.922f, 0.985f, 0.875f);

	auto light_perspective = glm::ortho(-70.0f, 70.0f, -70.0f, 70.0f, 0.0001f, 200.f);
	glm::mat4 light_view = glm::lookAt(glm::normalize(GetLightDirection()) * 40.0f, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	mp_light_transform_matrix = glm::fmat4(light_perspective * light_view);
}