#include "pch/pch.h"
#include "components/lights/DirectionalLight.h"
#include "glm/vec3.hpp"
#include "glm/gtx/transform.hpp"

void DirectionalLight::SetLightDirection(const glm::vec3& dir)
{
	m_light_direction = dir;

	glm::mat4 light_perspective = glm::ortho(-70.0f, 70.0f, -70.0f, 70.0f, 0.0001f, 200.f);
	glm::mat4 light_view = glm::lookAt(glm::normalize(dir) * 80.0f, glm::vec3(0), glm::vec3(0.0f, 1.0f, 0.0f));

	m_light_transform_matrix = glm::mat4(light_perspective * light_view);

}

DirectionalLight::DirectionalLight() : BaseLight(0) {
	diffuse_intensity = 1.f;
	color = glm::vec3(0.922f, 0.985f, 0.875f);

	auto light_perspective = glm::ortho(-70.0f, 70.0f, -70.0f, 70.0f, 0.0001f, 200.f);
	glm::mat4 light_view = glm::lookAt(glm::normalize(m_light_direction) * 80.0f, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	m_light_transform_matrix = glm::fmat4(light_perspective * light_view);
}