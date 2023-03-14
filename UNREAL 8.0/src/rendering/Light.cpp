#include "Light.h"
#include <glm/gtx/transform.hpp>


void SpotLight::SetLightDirection(float i, float j, float k) {
	m_light_direction_vec = glm::normalize(glm::fvec3(i, j, k));
	mesh_visual->SetRotation(glm::degrees(sinf(j)) - 90.0f, -glm::degrees(atan2f(k, i)) - 90.0f, 0.0f);

	float z_near = 0.1f;
	float z_far = GetMaxDistance();
	auto light_perspective = glm::perspective(glm::degrees(acosf(aperture)), 1.0f, z_near, z_far);
	auto spot_light_view = glm::lookAt(GetWorldTransform().GetPosition(), m_light_direction_vec * 50.0f, glm::vec3(0.0f, 1.0f, 0.0f));

	delete mp_light_transform_matrix;

	mp_light_transform_matrix = new glm::fmat4(light_perspective * spot_light_view);
}

SpotLight::SpotLight(const glm::fvec3& dir_vec, const float t_aperture) : m_light_direction_vec(dir_vec), aperture(cosf(glm::radians(t_aperture)))
{
	float z_near = 0.1f;
	float z_far = GetMaxDistance();
	auto light_perspective = glm::perspective(glm::degrees(acosf(aperture)), 1.0f, z_near, z_far);
	glm::mat4 spot_light_view = glm::lookAt(GetWorldTransform().GetPosition(), m_light_direction_vec, glm::vec3(0.0f, 1.0f, 0.0f));

	mp_light_transform_matrix = new glm::fmat4(light_perspective * spot_light_view);
	SetAttenuation(0.1f, 0.005f, 0.00001f);
	SetMaxDistance(480.0f);
};



void DirectionalLight::SetLightDirection(const glm::fvec3& dir)
{
	light_direction = dir;

	auto light_perspective = glm::ortho(-70.0f, 70.0f, -70.0f, 70.0f, 0.0001f, 200.f);
	glm::mat4 light_view = glm::lookAt(glm::normalize(dir) * 40.0f, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	delete mp_light_transform_matrix;
	mp_light_transform_matrix = new glm::fmat4(light_perspective * light_view);

}

DirectionalLight::DirectionalLight() {
	SetDiffuseIntensity(1.0f);
	SetColor(0.922f, 0.985f, 0.875f);

	auto light_perspective = glm::ortho(-70.0f, 70.0f, -70.0f, 70.0f, 0.0001f, 200.f);
	glm::mat4 light_view = glm::lookAt(glm::normalize(GetLightDirection()) * 40.0f, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	mp_light_transform_matrix = new glm::fmat4(light_perspective * light_view);
}